#include "clogic.h"

void CLogic::setNetPackMap()
{
    NetPackMap(_DEF_PACK_REGISTER_RQ)    = &CLogic::RegisterRq;
    NetPackMap(_DEF_PACK_LOGIN_RQ)       = &CLogic::LoginRq;
}

#define _DEF_COUT_FUNC_    cout << "clientfd:"<< clientfd <<" "<< __func__ << endl;
#define DEF_PATH "~/project/NetDisk/"
//处理客户端来的注册请求
void CLogic::RegisterRq(sock_fd clientfd,char* szbuf,int nlen)
{
    //cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_
   // 拆包  tel password name
    STRU_REGISTER_RQ *rq =(STRU_REGISTER_RQ*)szbuf;
    STRU_REGISTER_RS rs;
   //根据 tel 查看手机号是否存在
    char sqlstr[1000] = "";
    sprintf(sqlstr,"select u_tel from t_user where u_tel = '%s';",rq->tel);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlstr,1,lstRes);
    if (!res)
        std::cout << "select fail:"<<sqlstr<<std::endl;
    if(lstRes.size()!=0){
        //存在 返回
        rs.result = tel_is_exist;
    }else{
        //不存在 查看昵称是否存在
        sprintf(sqlstr,"select u_tel from t_user where u_name='%s';",rq->name);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlstr,1,lstRes);
        if (!res)
            std::cout << "select fail:"<<sqlstr<<std::endl;
        if(lstRes.size()!=0){
            //存在 返回
            rs.result = name_is_exist;
         }else{
            //不存在
            rs.result = register_success;
            //注册成功，写入数据库
            sprintf(sqlstr,"insert into t_user(u_tel , u_password , u_name) value ('%s','%s','%s');",rq->tel,rq->password,rq->name);
            if(!m_sql->UpdataMysql(sqlstr))
                std::cout << "updata fail:"<<sqlstr<<std::endl;
            //取出该人的Id
            sprintf(sqlstr,"select u_id from t_user where u_tel = '%s' and u_password = '%s';",rq->tel,rq->password);
            lstRes.clear();
            bool res = m_sql->SelectMysql(sqlstr,1,lstRes);
            if(!res)
               std::cout << "select fail:"<<sqlstr<<std::endl;
            if(lstRes.size()!=0){
                int id = stoi(lstRes.front());
                lstRes.pop_front();
             //网盘：创建该人对应的目录id命名
                //默认路径 ~/project/NetDisk/
                char pathbuf[_MAX_PATH]="";
                sprintf(pathbuf,"%s%d/",DEF_PATH,id);
                //创建路径
                umask(0);  //设置权限掩码为0 取消掉掩码的影响
                mkdir(pathbuf,0777);

            }

        }

    }
    SendData(clientfd,(char*)&rs,sizeof(rs));
}

//登录
void CLogic::LoginRq(sock_fd clientfd ,char* szbuf,int nlen)
{
//    cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_
    //拆包 tel password
    STRU_LOGIN_RQ *rq = (STRU_LOGIN_RQ*)szbuf;
    STRU_LOGIN_RS rs;
    //根据tel 查 id     password    name
    char sqlstr[1000] = "";
    sprintf(sqlstr,"select u_id,u_password,u_name from t_user where u_tel = '%s';",rq->tel);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlstr,3,lstRes);
    
    if(!res)
        std::cout << "select fail:"<<sqlstr<<std::endl;
    //没有
    if(lstRes.size()==0){
        rs.result = tel_not_exist;
    }
    //有
    else{
        //从数据库中取出
        int id = stoi(lstRes.front());
        lstRes.pop_front();
        string strPassword = lstRes.front();
        lstRes.pop_front();
        string strName = lstRes.front();
        lstRes.pop_front();
        //检测登录密码是否和数据库中一致
        if(strcmp(strPassword.c_str(),rq->password)!=0){
            //密码不一致返回
            rs.result = password_error;
        }else{
            //密码一致
            rs.result = login_success;
            rs.userid = id;
            strcpy(rs.name,strName.c_str());
            //用户身份
            //创建用户身份结构体
            //首先查看是否已经在Map中，如果不在直接创建
            UserInfo* info = nullptr;
            if(!m_mapIDToUserInfo.find(id,info)){
                info = new UserInfo;
            }else{
                //如果存在，考虑 ， 让其下线
            }
            //赋值
            info->name = strName;
            info->clientfd = clientfd;
            info->userid = id;
            //写入map
            m_mapIDToUserInfo.insert(id,info);
        }
    }
    SendData(clientfd,(char*)&rs,sizeof(rs));
}
