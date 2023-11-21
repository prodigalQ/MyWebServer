#ifndef HTTPROCESS_H
#define HTTPROCESS_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "buffer.h"

class HttpProcess{
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpProcess() { }
    ~HttpProcess(){
        unmapFile();
    }

    void initReq();
    void initRes(const std::string& srcDir,bool isKeepAlive,int code=-1);
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    bool IsKeepAlive() const;
    void makeResponse(Buffer& buffer);
    void unmapFile();
    char* file() {return mmFile_;}
    size_t fileLen() const {return mmFileStat_.st_size;}

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    bool ParseRequestLine(const std::string& line);
    void ParseHeader(const std::string& line);
    void ParseBody(const std::string& line);
    void ParsePath();
    void ParsePost();
    void ParseFromUrlencoded();

    void AddStateLine(Buffer &buff);
    void AddHeader(Buffer &buff);
    void AddContent(Buffer &buff);

    void errorHTML();
    void addStateLine(Buffer &buff);
    void addResponseHeader(Buffer &buff);
    void addResponseContent(Buffer &buff);
    void errorContent(Buffer& buff, std::string message);
    std::string getFileType();

    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_;
    int code_;
    std::string method_, path_, version_, body_, srcDir_;
    char* mmFile_;
    struct stat mmFileStat_;
    bool isKeepAlive_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
    static int ConverHex(char ch);
};

#endif