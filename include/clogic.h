#ifndef CLOGIC_H
#define CLOGIC_H
// #define _DEF_NUMBER 1000000000UL
#include"TCPKernel.h"
//目的是其他业务只需要改逻辑类和协议即可
class CLogic
{
public:
    CLogic( TcpKernel* pkernel )
    {
        m_pKernel = pkernel;
        m_sql = pkernel->m_sql;
        m_tcp = pkernel->m_tcp;
    }
public:
    //设置协议映射
    void setNetPackMap();
    //获取1000000000UL
    int getNumber(){
        return 1000000000;
    }
    /************** 发送数据*********************/
    void SendData( sock_fd clientfd, char*szbuf, int nlen )
    {
        m_pKernel->SendData( clientfd ,szbuf , nlen );
    }
    /************** 网络处理 *********************/
    //注册
    void RegisterRq(sock_fd clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(sock_fd clientfd, char*szbuf, int nlen);
    //上传文件
    void UploadFileRq(sock_fd clientfd, char*szbuf, int nlen);
    //文件块内容请求
    void FileContentRq(sock_fd clientfd, char*szbuf, int nlen);

    /*******************************************/

private:
    TcpKernel* m_pKernel;
    CMysql * m_sql;
    Block_Epoll_Net * m_tcp;

    MyMap<int , UserInfo*> m_mapIDToUserInfo;  //带锁的MyMap,存储登录的用户信息

    //key userid乘1000000000 + timestamp     value:文件信息
    MyMap<int64_t , FileInfo*> m_mapTimestampToFileInfo;
};

#endif // CLOGIC_H
