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


}
