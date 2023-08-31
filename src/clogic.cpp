#include "clogic.h"

void CLogic::setNetPackMap()
{
    NetPackMap(_DEF_PACK_REGISTER_RQ) = &CLogic::RegisterRq;
    NetPackMap(_DEF_PACK_LOGIN_RQ) = &CLogic::LoginRq;
    NetPackMap(_DEF_PACK_UPLOAD_FILE_RQ) = &CLogic::UploadFileRq;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RQ) = &CLogic::FileContentRq;


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
    }else{
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

        }
    }

    //返回给客户端结果
    rs.fileid = rq->fileid;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    rs.len = rq->len;
    SendData(clientfd , (char*)&rs , sizeof(rs));
}
