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
    //处理注册
    void RegisterRq(sock_fd clientfd, char*szbuf, int nlen);
    //处理登录
    void LoginRq(sock_fd clientfd, char*szbuf, int nlen);
    //处理上传文件请求
    void UploadFileRq(sock_fd clientfd, char*szbuf, int nlen);  //最后发送给客户端上传文件回复
    //处理文件块内容请求
    void FileContentRq(sock_fd clientfd, char*szbuf, int nlen);
    //处理获取文件信息请求
    void GetFileInfoRq(sock_fd clientfd, char*szbuf, int nlen);
    //处理下载文件请求
    void DownloadFileRq(sock_fd clientfd, char*szbuf, int nlen);  //最后发送文件头请求
    //处理下载文件夹请求
    void DownloadFolderRq(sock_fd clientfd, char*szbuf, int nlen);
    //处理客户端发来的文件头回复，处理客户端的下载文件文件头回复（客户端告知服务端已经创建，等待服务端传输内容，服务端开始传输））
    void FileHeaderRs(sock_fd clientfd, char*szbuf, int nlen);
    //处理客户端发来的文件内容回复
    void FileContentRs(sock_fd clientfd, char*szbuf, int nlen);
    //处理客户端发来的新建文件夹请求
    void AddFolderRq(sock_fd clientfd, char*szbuf, int nlen);
    //处理客户端发来的分享文件请求
    void ShareFileRq(sock_fd clientfd, char *szbuf, int nlen);
    //处理客户端发来的获取他所有分享文件请求
    void MyShareRq(sock_fd clientfd, char *szbuf, int nlen);
    //处理客户端发来的通过分享码获取分享请求
    void GetShareRq(sock_fd clientfd, char *szbuf, int nlen);

    void GetShareByFile(int userid, int fileid, string dir, string name, string time);

    void GetShareByFolder(int userid, int fileid, string dir, string name, string time, int fromuserid, string fromdir);


    /*******************************************/

    //分享一个文件
    void ShaerItem(int userid, int fileid, string dir, string time, int link);

private:
    TcpKernel* m_pKernel;
    CMysql * m_sql;
    Block_Epoll_Net * m_tcp;

    MyMap<int , UserInfo*> m_mapIDToUserInfo;  //带锁的MyMap,存储登录的用户信息
    

    //此map包含客户端某个用户在某个时间对哪个文件做了上传或者下载操作。上传或者下载成功后删除对应节点
    //key userid乘1000000000 + timestamp     value:文件信息
    MyMap<int64_t , FileInfo*> m_mapTimestampToFileInfo;
};

#endif // CLOGIC_H
