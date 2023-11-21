#include "../include/httprocess.h"
using namespace std;

const unordered_set<string> HttpProcess::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture" };

const unordered_map<string, int> HttpProcess::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

const std::unordered_map<std::string, std::string> HttpProcess::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpProcess::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpProcess::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

void HttpProcess::initReq() {
    method_ = path_ = version_ = body_ = srcDir_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
    code_ = -1;
    isKeepAlive_ =false;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
}

void HttpProcess::initRes(const std::string& srcDir,bool isKeepAlive, int code){
    srcDir_ = srcDir;
    isKeepAlive_ = isKeepAlive;
    code_ = code;
}

bool HttpProcess::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpProcess::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    // if(buff.getUsedSize() <= 0) {
    //     return false;
    // }
    while(buff.getUsedSize() && state_ != FINISH) {
        const char* lineEnd = search(buff.getReadPtr(), buff.getWritePtr(), CRLF, CRLF + 2);
        if(lineEnd == buff.getWritePtr())
            break; 
        std::string line(buff.getReadPtr(), lineEnd);
        //std::cout <<"state " << state_ << " parse line:" << line << std::endl;
        //std::cout << "state " << state_ << std::endl;
        switch(state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine(line)) {
                return false;
            }
            ParsePath();
            //std::cout << method_ << " " << path_ << " " << version_ << std::endl;
            break;    
        case HEADERS:
            ParseHeader(line);
            if(buff.getUsedSize() <= 2) {
                state_ = FINISH;
            }
            // for(auto it = header_.begin(); it != header_.end(); it++){
            //     std::cout << it->first << " " << it->second << endl;
            // }
            break;
        case BODY:
            ParseBody(line);
            break;
        default:
            break;
        }
        //if(lineEnd == buff.getWritePtr()) { break; }
        buff.updateReadIdx(lineEnd + 2);
    }
    return true;
}

void HttpProcess::ParsePath() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpProcess::ParseRequestLine(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    return false;
}

void HttpProcess::ParseHeader(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        isKeepAlive_ = IsKeepAlive();
        state_ = BODY;
    }
}

void HttpProcess::ParseBody(const string& line) {
    body_ = line;
    ParsePost();
    state_ = FINISH;
}

int HttpProcess::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

void HttpProcess::ParsePost() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded();
        // if(DEFAULT_HTML_TAG.count(path_)) {
        //     int tag = DEFAULT_HTML_TAG.find(path_)->second;
        //     if(tag == 0 || tag == 1) {
        //         bool isLogin = (tag == 1);
        //         if(UserVerify(post_["username"], post_["password"], isLogin)) {
        //             path_ = "/welcome.html";
        //         } 
        //         else {
        //             path_ = "/error.html";
        //         }
        //     }
        // }
    }   
}

void HttpProcess::ParseFromUrlencoded() {
    if(body_.size() == 0) { return; }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            //LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

// bool HttpProcess::UserVerify(const string &name, const string &pwd, bool isLogin) {
//     if(name == "" || pwd == "") { return false; }
//     LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
//     MYSQL* sql;
//     SqlConnRAII(&sql,  SqlConnPool::Instance());
//     assert(sql);
    
//     bool flag = false;
//     unsigned int j = 0;
//     char order[256] = { 0 };
//     MYSQL_FIELD *fields = nullptr;
//     MYSQL_RES *res = nullptr;
    
//     if(!isLogin) { flag = true; }
//     /* 查询用户及密码 */
//     snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
//     LOG_DEBUG("%s", order);

//     if(mysql_query(sql, order)) { 
//         mysql_free_result(res);
//         return false; 
//     }
//     res = mysql_store_result(sql);
//     j = mysql_num_fields(res);
//     fields = mysql_fetch_fields(res);

//     while(MYSQL_ROW row = mysql_fetch_row(res)) {
//         LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
//         string password(row[1]);
//         /* 注册行为 且 用户名未被使用*/
//         if(isLogin) {
//             if(pwd == password) { flag = true; }
//             else {
//                 flag = false;
//                 LOG_DEBUG("pwd error!");
//             }
//         } 
//         else { 
//             flag = false; 
//             LOG_DEBUG("user used!");
//         }
//     }
//     mysql_free_result(res);

//     /* 注册行为 且 用户名未被使用*/
//     if(!isLogin && flag == true) {
//         LOG_DEBUG("regirster!");
//         bzero(order, 256);
//         snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
//         LOG_DEBUG( "%s", order);
//         if(mysql_query(sql, order)) { 
//             LOG_DEBUG( "Insert error!");
//             flag = false; 
//         }
//         flag = true;
//     }
//     SqlConnPool::Instance()->FreeConn(sql);
//     LOG_DEBUG( "UserVerify success!!");
//     return flag;
// }

std::string HttpProcess::path() const{
    return path_;
}

std::string& HttpProcess::path(){
    return path_;
}
std::string HttpProcess::method() const {
    return method_;
}

std::string HttpProcess::version() const {
    return version_;
}

std::string HttpProcess::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpProcess::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}
////////////////////////////////////////////////////////////////////////////////////////////

void HttpProcess::makeResponse(Buffer& buff) {
    /* 判断请求的资源文件 */
    //std::cout << srcDir_ + path_ << " " << stat((srcDir_ + path_).data(), &mmFileStat_) << std::endl;
    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }
    //std::cout << code_ << std::endl;
    errorHTML();
    //std::cout << path_ << std::endl;
    addStateLine(buff);
    addResponseHeader(buff);
    addResponseContent(buff);
}

void HttpProcess::errorHTML() {
    if(CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

void HttpProcess::addStateLine(Buffer& buff) {
    std::string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.writeToBuffer("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpProcess::addResponseHeader(Buffer& buff) {
    buff.writeToBuffer("Connection: ");
    if(isKeepAlive_) {
        buff.writeToBuffer("keep-alive\r\n");
        buff.writeToBuffer("keep-alive: max=5, timeout=120\r\n");
    } else{
        buff.writeToBuffer("close\r\n");
    }
    buff.writeToBuffer("Content-type: " + getFileType() + "\r\n");
}

void HttpProcess::addResponseContent(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcFd < 0) { 
        errorContent(buff, "File NotFound!");
        return; 
    }

    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    //LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        errorContent(buff, "File NotFound!");
        return; 
    }
    mmFile_ = (char*)mmRet;
    close(srcFd);
    buff.writeToBuffer("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
    //buff.writeToBuffer(mmFile_, mmFileStat_.st_size);
}

std::string HttpProcess::getFileType(){
    /* 判断文件类型 */
    std::string::size_type idx = path_.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpProcess::errorContent(Buffer& buff, std::string message) 
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.writeToBuffer("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.writeToBuffer(body);
}

void HttpProcess::unmapFile(){
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}
