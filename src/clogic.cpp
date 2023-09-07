#include "clogic.h"

//设置响应客户端请求协议数组
void CLogic::setNetPackMap()
{
    NetPackMap(_DEF_PACK_REGISTER_RQ) = &CLogic::RegisterRq;
    NetPackMap(_DEF_PACK_LOGIN_RQ) = &CLogic::LoginRq;
    NetPackMap(_DEF_PACK_UPLOAD_FILE_RQ) = &CLogic::UploadFileRq;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RQ) = &CLogic::FileContentRq;
    NetPackMap(_DEF_PACK_GET_FILE_INFO_RQ) = &CLogic::GetFileInfoRq;
    NetPackMap(_DEF_PACK_DOWNLOAD_FILE_RQ) = &CLogic::DownloadFileRq;
    NetPackMap(_DEF_PACK_FILE_HEADER_RS) = &CLogic::FileHeaderRs;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RS) = &CLogic::FileContentRs;
    NetPackMap(_DEF_PACK_ADD_FOLDER_RQ) = &CLogic::AddFolderRq;
    NetPackMap(_DEF_PACK_SHARE_FILE_RQ) = &CLogic::ShareFileRq;
    NetPackMap(_DEF_PACK_MY_SHARE_RQ) = &CLogic::MyShareRq;
    NetPackMap(_DEF_PACK_GET_SHARE_RQ) = &CLogic::GetShareRq;
    NetPackMap(_DEF_PACK_DOWNLOAD_FOLDER_RQ) = &CLogic::DownloadFolderRq;
    NetPackMap(_DEF_PACK_DELETE_FILE_RQ) = &CLogic::DeleteFileRq;
    NetPackMap(_DEF_PACK_CONTINUE_DOWNLOAD_RQ) = &CLogic::ContinueDownloadRq;
    NetPackMap(_DEF_PACK_CONTINUE_UPLOAD_RQ) = &CLogic::ContinueUploadRq;
}

#define _DEF_COUT_FUNC_ cout << "clientfd:" << clientfd << " " << __func__ << endl;
#define DEF_PATH "/home/zhou/project/NetDisk/"
// 处理客户端来的注册请求
void CLogic::RegisterRq(sock_fd clientfd, char *szbuf, int nlen)
{
    // cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_
    // 拆包  tel password name
    STRU_REGISTER_RQ *rq = (STRU_REGISTER_RQ *)szbuf;
    STRU_REGISTER_RS rs;
    // 根据 tel 查看手机号是否存在
    char sqlstr[1000] = "";
    sprintf(sqlstr, "select u_tel from t_user where u_tel = '%s';", rq->tel);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlstr, 1, lstRes);
    if (!res)
        std::cout << "select fail:" << sqlstr << std::endl;
    if (lstRes.size() != 0)
    {
        // 存在 返回
        rs.result = tel_is_exist;
    }
    else
    {
        // 不存在 查看昵称是否存在
        sprintf(sqlstr, "select u_tel from t_user where u_name='%s';", rq->name);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlstr, 1, lstRes);
        if (!res)
            std::cout << "select fail:" << sqlstr << std::endl;
        if (lstRes.size() != 0)
        {
            // 存在 返回
            rs.result = name_is_exist;
        }
        else
        {
            // 不存在
            rs.result = register_success;
            // 注册成功，写入数据库
            sprintf(sqlstr, "insert into t_user(u_tel , u_password , u_name) value ('%s','%s','%s');", rq->tel, rq->password, rq->name);
            if (!m_sql->UpdataMysql(sqlstr))
                std::cout << "updata fail:" << sqlstr << std::endl;
            // 取出该人的Id
            sprintf(sqlstr, "select u_id from t_user where u_tel = '%s' and u_password = '%s';", rq->tel, rq->password);
            lstRes.clear();
            bool res = m_sql->SelectMysql(sqlstr, 1, lstRes);
            if (!res)
                std::cout << "select fail:" << sqlstr << std::endl;
            if (lstRes.size() != 0)
            {
                int id = stoi(lstRes.front());
                lstRes.pop_front();
                // 网盘：创建该人对应的目录id命名
                // 默认路径 ~/project/NetDisk/
                char pathbuf[_MAX_PATH] = "";
                sprintf(pathbuf, "%s%d/", DEF_PATH, id);
                // 创建路径
                umask(0); // 设置权限掩码为0 取消掉掩码的影响
                mkdir(pathbuf, 0777);
            }
        }
    }
    SendData(clientfd, (char *)&rs, sizeof(rs));
}

// 登录
void CLogic::LoginRq(sock_fd clientfd, char *szbuf, int nlen)
{
    //    cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_
    // 拆包 tel password
    STRU_LOGIN_RQ *rq = (STRU_LOGIN_RQ *)szbuf;
    STRU_LOGIN_RS rs;
    // 根据tel 查 id     password    name
    char sqlstr[1000] = "";
    sprintf(sqlstr, "select u_id,u_password,u_name from t_user where u_tel = '%s';", rq->tel);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlstr, 3, lstRes);

    if (!res)
        std::cout << "select fail:" << sqlstr << std::endl;
    // 没有
    if (lstRes.size() == 0)
    {
        rs.result = tel_not_exist;
    }
    // 有
    else
    {
        // 从数据库中取出
        int id = stoi(lstRes.front());
        lstRes.pop_front();
        string strPassword = lstRes.front();
        lstRes.pop_front();
        string strName = lstRes.front();
        lstRes.pop_front();
        // 检测登录密码是否和数据库中一致
        if (strcmp(strPassword.c_str(), rq->password) != 0)
        {
            // 密码不一致返回
            rs.result = password_error;
        }
        else
        {
            // 密码一致
            rs.result = login_success;
            rs.userid = id;
            strcpy(rs.name, strName.c_str());
            // 用户身份
            // 创建用户身份结构体
            // 首先查看是否已经在Map中，如果不在直接创建
            UserInfo *info = nullptr;
            if (!m_mapIDToUserInfo.find(id, info))
            {
                info = new UserInfo;
            }
            else
            {
                // 如果存在，考虑 ， 让其下线
            }
            // 赋值
            info->name = strName;
            info->clientfd = clientfd;
            info->userid = id;
            // 写入map
            
            m_mapIDToUserInfo.insert(id, info);
            std::cout<<"map m_mapIDToUserInfo insert success\n"<<std::endl;
        }
    }
    SendData(clientfd, (char *)&rs, sizeof(rs));
}

void CLogic::UploadFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    // 拆包
    STRU_UPLOAD_FILE_RQ *rq = (STRU_UPLOAD_FILE_RQ *)szbuf;
    // TODO 查看是否秒传
    {
        //花括号内可以独有变量
        // 根据 Md5 state = 1 查数据库 得到id
        // 如果state 是0怎么处理，客户端给拒绝 或者挂起这个请求
        char sqlbuf[1000] = "";
        sprintf(sqlbuf, "select f_id from t_file where f_MD5 = '%s' and f_state = 1 ;", rq->md5);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlbuf, 1, lstRes);
        if(!res){
            printf("select fail : %s\n" , sqlbuf);
            return;
        }
        // 判断这个文件是否已经上传过
        if(lstRes.size() > 0){
            int fileid = stoi(lstRes.front()); lstRes.pop_front();
            //插入用户文件关系表  由于右触发器  文件引用计数自动+1
            sprintf(sqlbuf,"insert into t_user_file (u_id,f_id,f_dir,f_name,f_uploadtime) values (%d,%d,'%s','%s','%s');"
                ,rq->userid , fileid , rq->dir , rq->fileName , rq->time
            );
            res = m_sql->UpdataMysql(sqlbuf);
            if(!res){
                printf("update fail : %s\n" , sqlbuf);
            }

            //写回复包 客户端收到之后，就更新列表
            STRU_QUICK_UPLOAD_RS rs;
            rs.result = 1;
            rs.timestamp = rq->timestamp;
            rs.userid=rq->userid;
            // 发送
            SendData(clientfd , (char*)&rs , sizeof(rs));
            //返回
            return;
        }
    }
    // 不是秒传
    // 创建文件 打开文件
    FileInfo *info =nullptr;
    info = new FileInfo;
    char strpath[1000] = "";
    sprintf(strpath, "%s%d%s%s", DEF_PATH, rq->userid, rq->dir, rq->md5);
    info->absolutePath = strpath; // 通过这个写数据库打开文件 以MD5作为文件名字
    info->dir = rq->dir;
    //info->fid;
    info->md5 = rq->md5;
    info->name = rq->fileName;
    info->size = rq->size;
    info->time = rq->time;
    info->type = rq->type;
    // info->fid=0;
    // info->pos = 0;
    // info->fileFd = 0;
    //使用Linux 文件io
    info->fileFd = open(strpath, O_CREAT | O_WRONLY | O_TRUNC, 00777);
    if(info->fileFd < 0){
        std::cout << "open file fail:" << strpath << std::endl;
        perror("open file fail:");
        return;
    }
    //map存储文件信息
    int64_t user_time = rq->userid * getNumber() + rq->timestamp;
    // std::cout << "map insert 准备:" << std::endl;
    // if(m_mapTimestampToFileInfo.find(user_time, info)){
    //     m_mapTimestampToFileInfo.erase(user_time);
    // };
    m_mapTimestampToFileInfo.insert(user_time, info);
    // std::cout << "map insert success:" << std::endl;
    //数据库记录
    //插入文件信息 （引用计数 0  状态 0   上传结束后状态改为1）
    char sqlbuf[1000] = "";
    sprintf(sqlbuf,"insert into t_file (f_size , f_path , f_MD5 , f_count , f_state , f_type) values (%d , '%s' , '%s' , 0, 0 , 'file');",rq->size , strpath , rq->md5);
    bool res = m_sql->UpdataMysql(sqlbuf);
    if(!res){
        printf("update fail : %s\n" , sqlbuf);
    }
    // else{
    //     printf("update success : %s\n" , sqlbuf);
    // }

    //查文件ID
    sprintf(sqlbuf , "select f_id from t_file where f_path = '%s' and f_MD5 = '%s';" , strpath,rq->md5);
    list<string> lstRes;
    res = m_sql->SelectMysql(sqlbuf , 1 , lstRes);
    if(!res){
        printf("select fail : %s\n" , sqlbuf);
    }
    if(lstRes.size()>0){
        info->fid = stoi(lstRes.front());
    }
    lstRes.clear();

    //插入用户文件关系表
    sprintf(sqlbuf,"insert into t_user_file (u_id,f_id,f_dir,f_name,f_uploadtime) values (%d,%d,'%s','%s','%s');"
        ,rq->userid , info->fid , rq->dir , rq->fileName , rq->time
    );
    res = m_sql->UpdataMysql(sqlbuf);
    if(!res){
        printf("update fail : %s\n" , sqlbuf);
    }

    //写回复 告诉客户端我这已经创建了文件，你可以传输文件内容了
    STRU_UPLOAD_FILE_RS rs;
    rs.fileid=info->fid;
    rs.result = 1;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    SendData(clientfd , (char*)&rs , sizeof(rs));
    printf("senddata success\n");
}

void CLogic::FileContentRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    // 拆包
    STRU_FILE_CONTENT_RQ *rq = (STRU_FILE_CONTENT_RQ *)szbuf;

    //获取文件信息
    int64_t user_time = rq->userid * getNumber() + rq->timestamp;
    FileInfo *info = nullptr;
    if(!m_mapTimestampToFileInfo.find(user_time , info)){
        printf("map find fail\n");
        return;
    }
    STRU_FILE_CONTENT_RS rs;
    //向文件中写入文件块
    int len  = write(info->fileFd , rq->content , rq->len);
    if(len != rq->len){
        //写入失败
        rs.result = 0;
        lseek(info->fileFd , -1*len , SEEK_CUR);
        cout << "失败调回" << endl;
    }
    else
    {
        //写入成功
        rs.result = 1;
        info->pos += len;
        if(info->pos >= info->size){
            //是否写完整个文件
            close(info->fileFd);
            //回收map节点
            m_mapTimestampToFileInfo.erase(user_time);
            delete info;
            info=nullptr;
            //更新数据库,把文件信息的状态更新为1 表示已完成
            char sqlbuf[1000] = "";
            sprintf(sqlbuf, "update t_file set f_state = 1 where f_id = %d;", rq->fileid);
            bool res = m_sql->UpdataMysql(sqlbuf);
            if(!res){
                cout << "update fail : " <<sqlbuf << endl;
            }
            else{
                cout << "update success : " <<sqlbuf << endl;
            }

        }
    }

    //返回给客户端结果
    rs.fileid = rq->fileid;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    rs.len = rq->len;
    SendData(clientfd , (char*)&rs , sizeof(rs));
    cout << "FileContentRq 发送"<<endl;
}

void CLogic::GetFileInfoRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_GET_FILE_INFO_RQ *rq = (STRU_GET_FILE_INFO_RQ*)szbuf;

    //根据 id dir 查表（视图）获取文件信息
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "select f_id,f_name,f_size,f_uploadtime,f_type from user_file_info where u_id = %d and f_dir ='%s' and f_state = 1;", rq->userID, rq->dir);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlbuf , 5 , lstRes);
    if(!res){
        printf("select fail : %s\n" , sqlbuf);
        return ;
    }
    if(lstRes.size() == 0){
        printf("不存在 : %s\n" , sqlbuf);
        return ;
    }
    int count = lstRes.size() / 5;
    // 写回复包
    int packlen = sizeof(STRU_GET_FILE_INFO_RS) + count * sizeof(STRU_FILE_INFO);
    //STRU_GET_FILE_INFO_RS *rs = (STRU_GET_FILE_INFO_RS*)new char[packlen];
    //malloc版本
    STRU_GET_FILE_INFO_RS *rs = (STRU_GET_FILE_INFO_RS*)malloc(packlen);

    rs->init();
    rs->count = count;
    strcpy(rs->dir , rq->dir);
    for (int i = 0; i < count; ++i)
    {
        int f_id = stoi(lstRes.front()); lstRes.pop_front();
        string name = lstRes.front(); lstRes.pop_front();
        int f_size = stoi(lstRes.front()); lstRes.pop_front();
        string time = lstRes.front(); lstRes.pop_front();
        string f_type = lstRes.front(); lstRes.pop_front();

        rs->fileInfo[i].fileID = f_id;
        strcpy(rs->fileInfo[i].name , name.c_str());
        rs->fileInfo[i].size = f_size;
        strcpy(rs->fileInfo[i].time , time.c_str());
        strcpy(rs->fileInfo[i].fileType , f_type.c_str());
    }

    //发送
    SendData(clientfd , (char*)rs , packlen);
    //回收
    free(rs);
    printf("getfileinforq success\n");
}
//处理下载文件请求
void CLogic::DownloadFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_DOWNLOAD_FILE_RQ *rq = (STRU_DOWNLOAD_FILE_RQ*)szbuf;
    //查数据库 查什么 为什么 f_name , f_path ,f_MD5 ,f_size
    char sqlbuf[1000] = "";
    sprintf(sqlbuf,"select f_name , f_path ,f_MD5 ,f_size from user_file_info where u_id = %d and f_dir = '%s' and f_id= %d;" , 
    rq->userid , rq->dir ,rq->fileid);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlbuf , 4 , lstRes);
    if(!res){
        std::cout << "select fail :" << sqlbuf << std::endl;
        return;
    }
    //如果没有返回
    if(lstRes.size() == 0){
        std::cout << "select zero :" << sqlbuf << std::endl;
        return;
    }
    //有 先取出 再写文件信息
    string strName = lstRes.front(); lstRes.pop_front();  //客户端看见的文件名vxxxxxr.jpg
    string strPath = lstRes.front(); lstRes.pop_front();  //服务器中文件的真实路径 /home/zhou/project/NetDisk/2/9372ef2fe33b0aa4dac6fbeb11bb16a8
    string strMD5 = lstRes.front(); lstRes.pop_front();   //9372ef2fe33b0aa4dac6fbeb11bb16a8 唯一标识文件名
    int size = stoi(lstRes.front()); lstRes.pop_front();

    FileInfo *info = new FileInfo;
    info->absolutePath = strPath;
    info->dir = rq->dir;
    info->fid = rq->fileid;
    info->md5 = strMD5;
    info->name = strName;
    info->size = size;
    info->type = "file";
    info->fileFd = open(info->absolutePath.c_str() , O_RDONLY);
    if(info->fileFd <= 0){
        std::cout << "open file fail :" << info->absolutePath << std::endl;
        return;
    }

    //key求出来
    int64_t user_time = rq->userid * getNumber() + rq->timestamp;
    //存到map里面
    m_mapTimestampToFileInfo.insert(user_time , info);

    //发送文件头请求，即通知客户端创建文件准备接收写入
    STRU_FILE_HEADER_RQ headrq;
    strcpy(headrq.dir, rq->dir);

    headrq.fileid = rq->fileid;
    headrq.timestamp = rq->timestamp;
    headrq.size = info->size;
    strcpy(headrq.md5,info->md5.c_str());
    strcpy(headrq.fileName,info->name.c_str());
    strcpy(headrq.fileType,"file");

    SendData(clientfd , (char*)&headrq , sizeof(headrq));
}

void CLogic::DownloadFolderRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_DOWNLOAD_FOLDER_RQ *rq = (STRU_DOWNLOAD_FOLDER_RQ*)szbuf;
    //查数据库 拿到信息 
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "select f_type ,f_id,f_name,f_path,f_MD5,f_size,f_dir from user_file_info where u_id = %d and f_dir = '%s' and f_id= %d;",
            rq->userid, rq->dir, rq->fileid);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlbuf, 7, lstRes);
    if(!res){
        cout<<"select fail : "<<sqlbuf<<endl;
        return;
    }
    string type = lstRes.front(); lstRes.pop_front();
    //此时必定是文件夹
    //有个问题是需要避免所有文件用一个时间戳！！！--->解决 引用
    int timestamp = rq->timestamp;
    DownloadFolder(rq->userid, timestamp, clientfd, lstRes);
}
void CLogic::DownloadFolder(int userid,int& timestamp , sock_fd clientfd, list<string> &lstRes)
{
    //string type = lstRes.front(); lstRes.pop_front();
    int fileid = stoi(lstRes.front());lstRes.pop_front();
    string strName = lstRes.front(); lstRes.pop_front();
    string strPath = lstRes.front(); lstRes.pop_front();
    string strMD5 = lstRes.front(); lstRes.pop_front();
    int size = stoi(lstRes.front()); lstRes.pop_front();
    string dir = lstRes.front(); lstRes.pop_front();

    //发送文件夹头请求
    STRU_FOLDER_HEADER_RQ rq;
    rq.timestamp = ++timestamp;
    strcpy(rq.dir, dir.c_str());
    rq.fileid = fileid;
    strcpy(rq.fileName , strName.c_str());
    SendData(clientfd, (char *)&rq, sizeof(rq));

    //拼接路径
    string newdir = dir + strName + "/";

    //查询下载当前文件夹内的内容列表
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "select f_type , f_id , f_name ,f_path , f_MD5,f_size,f_dir from user_file_info where u_id = %d and f_dir = '%s' ;",
            userid, newdir.c_str());
    list<string> newlstRes;
    bool res = m_sql->SelectMysql(sqlbuf, 7, newlstRes);
    if(!res){
        cout<<"select fail : "<<sqlbuf<<endl;
        return;
    }
    if(newlstRes.size() == 0){
        cout<<"select zero : "<<sqlbuf<<endl;
        return;
    }
    while(newlstRes.size() > 0){
        string type = newlstRes.front(); newlstRes.pop_front();
        if(type == "file"){
            DownloadFile(userid,timestamp,clientfd,newlstRes);
        }else{
            DownloadFolder(userid,timestamp,clientfd,newlstRes);
        }
    }
    

}
void CLogic::DownloadFile(int userid,int& timestamp , sock_fd clientfd, list<string> &lstRes)
{
    //取出数据库查到的信息
    int fileid = stoi(lstRes.front());lstRes.pop_front();
    string strName = lstRes.front(); lstRes.pop_front();
    string strPath = lstRes.front(); lstRes.pop_front();
    string strMD5 = lstRes.front(); lstRes.pop_front();
    int size = stoi(lstRes.front()); lstRes.pop_front();
    string dir = lstRes.front(); lstRes.pop_front();
    //赋值给文件信息结构体
    FileInfo *info = new FileInfo;
    info->absolutePath = strPath;
    info->dir = dir;
    info->fid = fileid;
    info->md5 = strMD5;
    info->name = strName;
    info->size = size;
    info->type = "file";
    info->fileFd = open(info->absolutePath.c_str() , O_RDONLY);
    if(info->fileFd <=0){
        cout << "file open fail" << endl;
        return;
    }
    //key求出来
    int64_t user_time = userid * getNumber() + (++timestamp);
    //存到map里面
    m_mapTimestampToFileInfo.insert (user_time , info);
    //发送文件头请求
    STRU_FILE_HEADER_RQ headrq;
    strcpy(headrq.dir, dir.c_str());
    headrq.fileid = fileid;
    strcpy( headrq.fileName , info->name.c_str());
    strcpy(headrq.md5, info->md5.c_str());
    strcpy(headrq.fileType, "file");
    headrq.size = info->size;
    headrq.timestamp = timestamp;
    SendData(clientfd , (char*)&headrq , sizeof(headrq));
}

//
void CLogic::ContinueDownloadRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_CONTINUE_DOWNLOAD_RQ *rq = (STRU_CONTINUE_DOWNLOAD_RQ*)szbuf;
    //看map是否存在文件信息
    int64_t user_time = rq->userid * getNumber() + rq->timestamp;
    FileInfo *info = nullptr;
   
    if (!m_mapTimestampToFileInfo.find(user_time, info)){
            // 没有创建文件信息 
        info = new FileInfo;
        // --由查表获取文件信息 添加到Map
        //查数据库 查什么 为什么 f_name , f_path ,f_MD5 ,f_size 通过 userid dir fileid 可以确定
        char sqlbuf[1000] = "";
        sprintf(sqlbuf,"select f_name , f_path ,f_MD5 ,f_size from user_file_info where u_id = %d and f_dir = '%s' and f_id= %d;" , 
        rq->userid , rq->dir ,rq->fileid);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlbuf , 4 , lstRes);
        if(!res){
            std::cout << "select fail :" << sqlbuf << std::endl;
            return;
        }
        //如果没有返回
        if(lstRes.size() == 0){
            std::cout << "select zero :" << sqlbuf << std::endl;
            return;
        }
        //有 先取出 再写文件信息
        string strName = lstRes.front(); lstRes.pop_front();  //客户端看见的文件名vxxxxxr.jpg
        string strPath = lstRes.front(); lstRes.pop_front();  //服务器中文件的真实路径 /home/zhou/project/NetDisk/2/9372ef2fe33b0aa4dac6fbeb11bb16a8
        string strMD5 = lstRes.front(); lstRes.pop_front();   //9372ef2fe33b0aa4dac6fbeb11bb16a8 唯一标识文件名
        int size = stoi(lstRes.front()); lstRes.pop_front();

        FileInfo *info = new FileInfo;
        info->absolutePath = strPath;
        info->dir = rq->dir;
        info->fid = rq->fileid;
        info->md5 = strMD5;
        info->name = strName;
        info->size = size;
        info->type = "file";
        info->fileFd = open(info->absolutePath.c_str() , O_RDONLY);
        if(info->fileFd <= 0){
            std::cout << "open continue file fail :" << info->absolutePath << std::endl;
            return;
        }
        m_mapTimestampToFileInfo.insert(user_time, info);
    }
    //有map中有文件信息了
    // 文件指针跳转  pos位置  同步Pos
    lseek(info->fileFd,rq->pos,SEEK_SET);//SET是起始位置
    info->pos = rq->pos;
    // 读文件块  发送文件块
    STRU_FILE_CONTENT_RQ contentRq;
    contentRq.fileid = rq->fileid;
    contentRq.timestamp = rq->timestamp;
    contentRq.userid = rq->userid;
    contentRq.len = read( info->fileFd , contentRq.content, _DEF_BUFFER);
    SendData(clientfd,(char*)&contentRq,sizeof(contentRq));
}

void CLogic::ContinueUploadRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_CONTINUE_UPLOAD_RQ *rq = (STRU_CONTINUE_UPLOAD_RQ *)szbuf;
    int64_t user_time = rq->userid * getNumber() + rq->timestamp;
    // 需要查看Map中是否存在 user_time
    FileInfo *info = nullptr;
    if(!m_mapTimestampToFileInfo.find(user_time , info)){
        // 不存在 创建
        //不需要查表的
        info = new FileInfo;
        info->dir = rq->dir;
        info->fid = rq->fileid;
        info->type = "file";
        //查表 获取信息 
        char sqlbuf[1000] = "";
        sprintf(sqlbuf, "select f_name , f_path  ,f_size ,f_MD5 from user_file_info where u_id = %d and f_id= %d and f_dir = '%s' and f_state = 0;" , rq->userid, rq->fileid, rq->dir);
        list<string> lst;
        bool res = m_sql->SelectMysql(sqlbuf, 4, lst);
        if(!res){
            cout << "select fail:" << sqlbuf << endl;
            return;
        }
        if(lst.size()==0)
            return;
        //给 info赋值 然后打开文件
        info->name = lst.front();           lst.pop_front();
        info->absolutePath = lst.front();   lst.pop_front();
        info->size = stoi(lst.front());     lst.pop_front();
        info->md5 = lst.front();            lst.pop_front();
        // info->fileFd = open(info->absolutePath.c_str(), O_WRONLY|O_APPEND);
        info->fileFd = open(info->absolutePath.c_str(), O_WRONLY);  //O_WRONLY不会清空
        if(info->fileFd <=0){
            cout << "file open fail:" << errno << endl;
            return;
        }
        m_mapTimestampToFileInfo.insert(user_time , info);
    }
    //现在已经有这个信息了  lseek 跳转并读取文件当前写的位置（就是文件末尾） 更新 Pos
    info->pos = lseek(info->fileFd, 0, SEEK_END);
    // 写回复 返回
    STRU_CONTINUE_UPLOAD_RS rs;
    rs.fileid = rq->fileid;
    rs.timestamp = rq->timestamp;
    rs.pos = info->pos;

    SendData(clientfd , (char*)&rs , sizeof(rs));
    
}

//处理客户端的下载文件文件头回复（客户端告知服务端已经创建，等待服务端传输内容，服务端开始传输）
void CLogic::FileHeaderRs(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_

    //拆包
    STRU_FILE_HEADER_RS *rs = (STRU_FILE_HEADER_RS*)szbuf;
    //拿到文件信息
    int64_t user_time = rs->userid * getNumber() + rs->timestamp;
    FileInfo *info = nullptr;
    if(!m_mapTimestampToFileInfo.find(user_time , info)){
        printf("map find fail\n");
        return;
    }

    //发送文件内容请求
    STRU_FILE_CONTENT_RQ rq;
    //读文件 到 rq.content 中 预计读4096个字节  rq.len为实际读了多少
    rq.len = read(info->fileFd, rq.content , _DEF_BUFFER);
    if(rq.len < 0){
        perror("read file fail:");
        return;
    }
    rq.fileid = rs->fileid;
    rq.timestamp = rs->timestamp;
    rq.userid = rs->userid;

    SendData(clientfd , (char*)&rq , sizeof(rq));
}

void CLogic::FileContentRs(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_FILE_CONTENT_RS *rs = (STRU_FILE_CONTENT_RS*)szbuf;

    //找到文件信息结构
    int64_t user_time = rs->userid * getNumber() + rs->timestamp;
    FileInfo *info = nullptr;
    if(!m_mapTimestampToFileInfo.find(user_time , info)){
        printf("map find fail\n");
        return;
    }
    //判断上次传输内容是否成功
    if(rs->result !=1){
        //不成功 跳回去
        lseek(info->fileFd , -1*(rs->len ), SEEK_CUR);
    }else{
        //成功
        info->pos += rs->len;
        //判断是否结束
        if(info->pos >= info->size){
            //是 关闭文件 放回 退出
            close(info->fileFd);
            m_mapTimestampToFileInfo.erase(user_time);
            delete info;
            info = nullptr;
            return;
        }
    }
    //没发完 继续写文件内容请求
    STRU_FILE_CONTENT_RQ rq;
    //读文件 到 rq.content 中 预计读4096个字节  rq.len为实际读了多少
    rq.len = read(info->fileFd, rq.content , _DEF_BUFFER);
    if(rq.len == 0)
        return;
    if(rq.len < 0){
        perror("read file fail:");
        return;
    }
    rq.fileid = rs->fileid;
    rq.timestamp = rs->timestamp;
    rq.userid = rs->userid;

    //发送
    SendData(clientfd , (char*)&rq , sizeof(rq));
}

void CLogic::AddFolderRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_ADD_FOLDER_RQ *rq = (STRU_ADD_FOLDER_RQ*)szbuf;

    //数据库写表
    char pathbuf[1000] = "";
    sprintf(pathbuf,"%s%d%s%s",DEF_PATH , rq->userid , rq->dir , rq->fileName);   ///home/zhou/project/NetDisk/id/dir/name

    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "insert into t_file (f_size,f_path,f_count,f_MD5,f_state,f_type) values (0,'%s',0,'?',1,'folder');", pathbuf);
    bool res = m_sql->UpdataMysql(sqlbuf);
    if(!res){
        printf("update fail : %s\n" , sqlbuf);
        return;
    }
    //查询id
    sprintf(sqlbuf, "select f_id from t_file where f_path = '%s';" ,pathbuf);
    list<string> lstRes;
    res = m_sql->SelectMysql(sqlbuf, 1, lstRes);
    if(!res){
        printf("select fail : %s\n" , sqlbuf);
        return;
    }
    if(lstRes.size() == 0){
        printf("select zero : %s\n" , sqlbuf);
        return;
    }
    int id = stoi(lstRes.front()); lstRes.pop_front();
    //写入用户文件关系  触发器引用计数会+1
    sprintf(sqlbuf, "insert into t_user_file (u_id,f_id,f_dir,f_name,f_uploadtime) values (%d,%d,'%s','%s','%s');"
        , rq->userid , id , rq->dir , rq->fileName , rq->time);
    res=m_sql->UpdataMysql(sqlbuf);
    if(!res){
        printf("update fail : %s\n" , sqlbuf);
        return;
    }

    //穿件目录
    umask(0);
    mkdir(pathbuf , 0777);
    
    //写回复
    STRU_ADD_FOLDER_RS rs;
    rs.result = 1;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    //发送
    SendData(clientfd , (char*)&rs , sizeof(rs));
}

void CLogic::ShareFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_SHARE_FILE_RQ *rq = (STRU_SHARE_FILE_RQ*)szbuf;
    //随机生成分享链接
    //分享码规则 9为分享码
    int link = 0;
    do{
        link = 1 + random() % 9;
        link *= 100000000;
        link +=random() % 100000000;
        //去重 查看连接是否已经存在
        char sqlbuf[1000] = "";
        sprintf(sqlbuf, "select s_link from t_user_file where s_link = %d;", link);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlbuf, 1, lstRes);
        if(!res){
            printf("select fail : %s\n" , sqlbuf);
            return;
        }
        if(lstRes.size() > 0){
            printf("link码已存在");
            link = 0;
        }
    }while(link == 0);
    //遍历 所有文件 ， 将其分享链接设置
    int itemCount = rq->itemCount;
    cout<<"分享项目数itemCount="<<itemCount<<endl;
    for (int i = 0; i < itemCount;++i){
        ShaerItem(rq->userid, rq->fileidArray[i], rq->dir, rq->shareTime, link);
        cout<<"用户id"<<rq->userid<<"分享文件id"<<rq->fileidArray[i]<<"分享时间"<<rq->shareTime<<"分享链接"<<link<<endl;
    }
    //写回复
    STRU_SHARE_FILE_RS rs;
    rs.result = 1;
    SendData(clientfd,(char*)&rs , sizeof(rs));
}

//处理客户端发来的获取他所有分享文件请求
void CLogic::MyShareRq(sock_fd clientfd, char *szbuf, int nlen)
{
    //拆包
    STRU_MY_SHARE_RQ *rq = (STRU_MY_SHARE_RQ*)szbuf;
    // rq->userid;
    //根据id 查询 获得分享文件列表
    //查的内容 f_name f_size s_linkTime s_link
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "select f_name ,f_size , s_linkTime , s_link from user_file_info where u_id = %d and s_link is not null and s_linkTime is not null;", rq->userid);
    list<string> lst;
    bool res = m_sql->SelectMysql(sqlbuf, 4, lst);
    if(!res){
        cout << "select fail\n" << sqlbuf << endl;
        return;
    }
    int count = lst.size();
    if( (count/4==0 )||(count % 4!= 0) )
        return;
    count /= 4;
    // 写回复
    int packlen = sizeof(STRU_MY_SHARE_RS) + count * sizeof(STRU_MY_SHARE_FILE);

    STRU_MY_SHARE_RS *rs = (STRU_MY_SHARE_RS*)malloc(packlen);
    rs->init();
    rs->itemCount = count;
    for (int i = 0; i < count;i++){
        string name = lst.front(); lst.pop_front();
        int size = stoi(lst.front()); lst.pop_front(); 
        string time = lst.front(); lst.pop_front();
        int link = stoi(lst.front()); lst.pop_front();

        strcpy(rs->items[i].name , name.c_str());
        rs->items[i].size = size;
        strcpy(rs->items[i].time , time.c_str());
        rs->items[i].shareLink = link;
    }
    //发送
    SendData(clientfd , (char*)rs , packlen);
    free(rs);
}

//处理客户端发来的通过分享码获取分享请求
void CLogic::GetShareRq(sock_fd clientfd, char *szbuf, int nlen)
{

    _DEF_COUT_FUNC_
    //拆包
    STRU_GET_SHARE_RQ *rq = (STRU_GET_SHARE_RQ*)szbuf;

    //根据分享码 查询f_id f_name f_dir(分享人) f_type u_id(分享人的)
    /*select f_id ,f_name , f_dir ,f_type ,u_id from user_file_info where s_link = 181315416*/
    char sqlbuf[1000] = "";
    sprintf(sqlbuf , "select f_id , f_name , f_dir ,f_type , u_id from user_file_info where s_link = %d;" , rq->shareLink);
    list<string> lst;
    bool res = m_sql->SelectMysql(sqlbuf, 5, lst);
    if(!res){
        cout << "select fail :" << sqlbuf << endl;
        return;
    }
    STRU_GET_SHARE_RS rs;
    if(lst.size()==0){
        //分享码无效或不存在
        rs.result = 0;
        SendData(clientfd , (char*)&rs , sizeof(rs));
        return;
    }

    rs.result = 1;
    if(lst.size() % 5 != 0)
        return;
    //遍历文件列表
    while(lst.size()!=0){
        int fileid = stoi(lst.front()); lst.pop_front();
        string name = lst.front(); lst.pop_front();
        string fromdir =lst.front(); lst.pop_front();
        string type =lst.front(); lst.pop_front();
        int fromuserid=stoi(lst.front()); lst.pop_front();

        if (type == "file"){
            //如果是文件
            //插入信息到 用户文件关系表//insert into t_user_file
            GetShareByFile(rq->userid, fileid, rq->dir, name, rq->time);
        }
        else
        {
            //如果是文件夹
            //插入信息到用户文件关系表
            //拼接路径 获取人目录 /-> /06/   分享人的目录 /->/06/
            //根据新路径 在分享人那边查询 文件夹下的文件
            //遍历链表 ->递归
            GetShareByFolder(rq->userid, fileid, rq->dir, name, rq->time, fromuserid, fromdir);
        }
    }
    //写回复包
    strcpy(rs.dir , rq->dir);
    //发送
    SendData(clientfd,(char*)&rs , sizeof(rs));
}

void CLogic::DeleteFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_DELETE_FILE_RQ * rq = (STRU_DELETE_FILE_RQ*)szbuf;
    //id列表
    for (int i = 0; i < rq->fileCount;++i){
        int fileid = rq->fileidArray[i];
        //删除每一项
        DeleteOneItem(rq->userid, fileid, rq->dir);
    }
    
    //写回复
    STRU_DELETE_FILE_RS rs;
    rs.result = 1;
    strcpy(rs.dir , rq->dir);
    SendData(clientfd , (char*)&rs , sizeof(rs));
}
void CLogic::DeleteOneItem(int userid , int fileid ,string dir)
{
    //删除文件需要 u_id f_dir f_id
    
    //需要知道是什么类型 type name（文件夹需要name）   path
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "select f_type ,f_name ,f_path from user_file_info where u_id = %d and f_id=%d and f_dir ='%s';"
        ,userid, fileid,dir.c_str());
    list<string> lst;
    bool res = m_sql->SelectMysql(sqlbuf, 3, lst);
    if(!res){
        cout<<"selectMysql fail:"<<sqlbuf<<endl;
        return;
    }
    if(lst.size()==0){
        cout<<"select zero:"<<sqlbuf<<endl;
        return;
    }
    string type = lst.front(); lst.pop_front();
    string name = lst.front(); lst.pop_front();
    string path = lst.front(); lst.pop_front();
    if(type == "file"){
        DeleteFile(userid, fileid, dir, path);
    }
    else{
        DeleteFolder(userid, fileid, dir, name);
    }

}
void CLogic::DeleteFile(int userid ,int fileid , string dir , string path)
{
    //删除用户文件对应的关系
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "delete from t_user_file where u_id = %d and f_dir = '%s' and f_id = %d;", userid, dir.c_str(), fileid);
    bool res = m_sql->UpdataMysql(sqlbuf); 
    if(!res){
        cout << "delete  fail : " << sqlbuf << endl;
        return;
    }
    //再次查询id看能不能找到数据库记录，如果不能，删除本地文件
    sprintf(sqlbuf,"select f_id from t_file where f_id = %d;",fileid);
    list<string> lst;
    res = m_sql->SelectMysql(sqlbuf, 1, lst);
    if(!res){
        cout<<"selectMysql fail:"<<sqlbuf<<endl;
        return;
    }
    if(lst.size()==0){
        unlink(path.c_str());  //linux 文件io 删除文件
    }
}
void CLogic::DeleteFolder(int userid ,int fileid , string dir ,string name)
{
    //删除用户文件对应的关系 u_id f_dir f_id
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "delete from t_user_file where u_id = %d and f_dir = '%s' and f_id = %d;", userid, dir.c_str(), fileid);
    bool res = m_sql->UpdataMysql(sqlbuf); 
    if(!res){
        cout << "delete  fail : " << sqlbuf << endl;
        return;
    }

    //拼接新路径
    std::string newdir = dir + name + "/";
    //查表 根据新路径查表 得到列表 f_id f_type path

    sprintf(sqlbuf, "select f_type ,f_id, f_name ,f_path from user_file_info where u_id = %d  and f_dir ='%s';", userid,newdir.c_str());
    list<string> lst;
    res = m_sql->SelectMysql(sqlbuf, 4, lst);
    if(!res){
        cout<<"selectMysql fail:"<<sqlbuf<<endl;
        return;
    }
    if(lst.size()==0){
        cout<<"select zero:"<<sqlbuf<<endl;
        return;
    }
        //循环
    while(lst.size()!=0){
        //取出信息
        string type = lst.front(); lst.pop_front();
        int fileid = stoi(lst.front()); lst.pop_front();
        string name =lst.front(); lst.pop_front();
        string path =lst.front(); lst.pop_front();

        // 如果是文件
        if(type=="file"){
            DeleteFile(userid, fileid, newdir, path);
        }
        // 如果是文件夹
        else{
            DeleteFolder(userid, fileid, newdir, name);
        }
    }
}

void CLogic::GetShareByFile(int userid,int fileid,string dir,string name ,string time){
    char sqlbuf[1000] = "";
    //客户端请求分享的时间作为上传时间
    sprintf(sqlbuf, "insert into t_user_file (u_id,f_id,f_dir,f_name,f_uploadtime) values (%d,%d,'%s','%s','%s');"
        , userid , fileid , dir.c_str() , name.c_str() , time.c_str());
    bool res= m_sql->UpdataMysql(sqlbuf);
    if(!res){
        printf("update fail : %s\n" , sqlbuf);
    }

}

void CLogic::GetShareByFolder(int userid,int fileid,string dir,string name ,string time,
        int fromuserid ,string fromdir)
{
            //是文件夹
            //插入信息到用户文件关系表
    GetShareByFile(userid, fileid, dir, name, time);
    // 拼接路径 获取人目录 /-> /06/  
    string newdir = dir + name + "/";
    //分享人的目录 /->/06/
    string newfromdir = fromdir + name + "/";
    // 根据新路径 在分享人那边查询 文件夹下的文件
    char sqlbuf[1000] = "";
    sprintf(sqlbuf , "select f_id , f_name  ,f_type  from user_file_info where u_id = %d and f_dir = '%s';" 
                                                                    ,fromuserid,newfromdir.c_str());
    list<string> lst;
    bool res = m_sql->SelectMysql(sqlbuf, 3, lst);
    if(!res){
        cout << "select fail :" << sqlbuf << endl;
        return;
    }
    // 遍历链表 ->递归
    while(lst.size()!=0){
        int fileid=stoi(lst.front()); lst.pop_front();
        string name = lst.front(); lst.pop_front();
        string type = lst.front(); lst.pop_front();
        
        //是文件
        if(type == "file"){
            GetShareByFile(userid, fileid, newdir, name, time);
        }
        //是文件夹
        else{
            GetShareByFolder(userid, fileid, newdir, name, time, fromuserid, newfromdir);
        }
    }
}

void CLogic::ShaerItem(int userid, int fileid, string dir, string time, int link)
{
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "update t_user_file set s_link ='%d',s_linkTime = '%s' where u_id = %d and f_id = %d and f_dir ='%s';", link, time.c_str(), userid, fileid, dir.c_str());
    bool res = m_sql->UpdataMysql(sqlbuf);
    if(!res){
        cout << "UpdataMysql fail\n" << sqlbuf << endl;
        return;
    }
}
