#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <cstdint>
#include <vector>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <windows.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <functional>
#include <memory>
#include <utility>
#include <cstdlib>
#include <boost/algorithm/string/classification.hpp> 
#include <boost/algorithm/string/split.hpp> 
#include <boost/algorithm/string.hpp>
#include "./Projekt/Projekt/base64.h"

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

mongocxx::instance inst{}; // This should be done only once.
mongocxx::client conn{
   mongocxx::uri{
      "mongodb://localhost:27017/cloud"
   }

};
mongocxx::database db = conn["cloud"];
mongocxx::collection coll = db["fs.files"];
mongocxx::collection chunks = db["fs.chunks"];

using boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace pt = boost::property_tree;

std::string LongToString(int64_t longDate) {
    char buff[128];
    std::tm bt{};

    std::chrono::duration<int64_t, std::milli> dur(longDate);
    auto tp = std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(dur));
    std::time_t in_time_t = std::chrono::system_clock::to_time_t(tp);
    localtime_s(&bt, &in_time_t);
    strftime(buff, 128, "%Y-%m-%d %H:%M:%S", &bt);
    std::string resDate(buff);

    return resDate;

}




class my_server_session : public std::enable_shared_from_this<my_server_session> {
private:
    friend class my_server;
    boost::asio::ip::tcp::socket socket_;
    
    
    void do_read() {
        auto self(shared_from_this());
        try {
            boost::asio::async_read_until(socket_, buf, "\r\n\r\n", [this, self](boost::system::error_code ec,
                std::size_t transferred)
                {
                    if(!ec)
                    {
                        std::string metoda, link;

                        const std::string delimiter = "\r\n\r\n";



                        std::string command{
                            boost::asio::buffers_begin(buf.data()),
                            boost::asio::buffers_begin(buf.data()) + transferred - delimiter.length()
                        };
                        std::string delimiters("\n");
                        std::vector<std::string> parts;
                        std::vector<std::pair<std::string, std::string>> partkeyvalue;
                        boost::split(parts, command, boost::is_any_of(delimiters));

                        for (auto i : parts) {
                            std::string key = i.substr(0, i.find(":"));
                            std::string val = i.substr(i.find(":") + 1, i.length());

                            partkeyvalue.push_back(std::make_pair(key, val));

                        }



                        metoda = partkeyvalue.front().second.substr(0, partkeyvalue.front().second.find(" "));
                        link = partkeyvalue.front().second.substr(partkeyvalue.front().second.find(" ") + 1, partkeyvalue.front().second.length());
                        link = link.substr(0, link.find(" "));

                        std::cout << "Link : " <<metoda<<" " << link << std::endl;

                        buf.consume(transferred);

                        if (link.substr(0, 11) == "/api/files/" && metoda == "DELETE") {
                            auto id = link.substr(link.find_last_of("/") + 1);
                            do_delete(id);
                        }
                        else if (link.substr(0, 11) == "/api/files/" && metoda == "GET") {
                            auto id = link.substr(link.find_last_of("/") + 1);
                          
                            if (link.substr(11, 7) == "preview") {
                                do_download(id, true);
                            }
                            else {
                                do_download(id, false);
                            }
                        }
                        else if (link.substr(0, 10) == "/api/files" && metoda == "GET") {
                            response1(link);
                        }
                        else if (link.substr(0, 10) == "/api/files" && metoda == "POST") {
                            long long contlen;

                            for (auto& i : partkeyvalue) {

                                if (boost::algorithm::to_lower_copy(i.first) == "content-length") {
                                    contlen = std::stoll(i.second, 0);
                                    break;
                                }
                            }

                            do_postfile(contlen);
                        }

                    }

                });
        }
        catch (...) {
            socket_.close();
        }
    }

    void read_post_header(long long& conlength){
        auto self(shared_from_this());
        const std::string delimiter = "\r\n\r\n";
        try {
            boost::asio::async_read_until(socket_, buf, delimiter, [this, self, conlength](boost::system::error_code ec,
                std::size_t transferred)
                {
                    if (!ec) {
                        std::string command{
                                boost::asio::buffers_begin(buf.data()),
                                boost::asio::buffers_begin(buf.data()) + transferred
                        };
                        buf.consume(transferred);
                        read_body(command, conlength);
                    }
                });
        }
        catch (...) {
            socket_.close();
        }  
    }
    void read_body(std::string& headers, long long conlength) {
        int chunk_size = 261120;

        std::string line1 = headers.substr(0, headers.find("\r\n"));
        std::string tmp = headers.substr(headers.find("\r\n")+1);
        std::string line2 = tmp.substr(0,tmp.find("\r\n"));
        line2 = line2.substr(line2.find("filename="));
        line2 = line2.substr(10, line2.length() - 11);
        tmp = tmp.substr(tmp.find("\r\n") + 1);
        std::string line3 = tmp.substr(0, tmp.find("\r\n"));
        line3 = line3.substr(line3.find(":")+2);
  

        int header_length = headers.length() + line1.length() + 6;
        long long file_length = conlength - header_length;

        long long n_chunks = (file_length + chunk_size - 1) / chunk_size;
        int last_chunk_size = file_length - chunk_size * (n_chunks - 1);
        
        auto start = std::chrono::system_clock::now();
    
        auto end = std::chrono::system_clock::now();
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    

        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(document{} << "length" << bsoncxx::types::b_int64{ file_length }
            << "chunkSize" << chunk_size
            << "uploadDate" << bsoncxx::types::b_date{
                std::chrono::system_clock::from_time_t(end_time)
            }
            << "filename" << line2
            << "contentType" << line3
            << bsoncxx::builder::stream::finalize);

        std::string inserted_file_id = (*result).inserted_id().get_oid().value.to_string();
 
        
        read_chunks(n_chunks, 0, chunk_size, last_chunk_size, bsoncxx::oid(inserted_file_id));
    }
        void read_chunks(long long n_chunks,long long n, int chunk_size, int last_chunk_size, bsoncxx::oid file_id) {
        auto self(shared_from_this());
        try {
            if (n_chunks > 1) {
          
                boost::asio::async_read(socket_, buf, boost::asio::transfer_exactly(chunk_size), [this, self, n, n_chunks, chunk_size, last_chunk_size, file_id](boost::system::error_code ec,
                    std::size_t transferred)
                    {
                        if (!ec) {
                            std::string command{
                                    boost::asio::buffers_begin(buf.data()),
                                    boost::asio::buffers_begin(buf.data()) + transferred
                            };


                            const std::uint8_t* pString = (uint8_t*)command.c_str();
                            bsoncxx::types::b_binary bin_data;
                            bin_data.size = chunk_size;

                            bin_data.bytes = pString;
                            bin_data.sub_type = bsoncxx::binary_sub_type::k_binary;

                            bsoncxx::stdx::optional<mongocxx::result::insert_one> result = chunks.insert_one(document{}
                                << "files_id" << file_id
                                << "n" << n
                                << "data" << bin_data
                                << bsoncxx::builder::stream::finalize);

                            buf.consume(transferred);

                            read_chunks(n_chunks - 1, n + 1, chunk_size, last_chunk_size, file_id);
                        }
                        else {
                            do_delete(file_id.to_string());
                        }
                    });
            }
            else if (n_chunks == 1) {
                int bytes_to_transfer = last_chunk_size - buf.size();
                std::string command{
                                boost::asio::buffers_begin(buf.data()),
                                boost::asio::buffers_begin(buf.data()) + buf.size()
                };
                buf.consume(buf.size());
                if (bytes_to_transfer > 0) {
                    boost::asio::async_read(socket_, buf, boost::asio::transfer_exactly(bytes_to_transfer), [this, self, n, command, n_chunks, chunk_size, last_chunk_size, file_id](boost::system::error_code ec,
                        std::size_t transferred)
                        {
                            if (!ec) {
                                std::string command2{
                                        boost::asio::buffers_begin(buf.data()),
                                        boost::asio::buffers_begin(buf.data()) + transferred
                                };
                                std::string command3 = command + command2;


                                std::string encoded_res = base64_encode(command3);

                                const std::uint8_t* pString = (uint8_t*)command3.c_str();
                                bsoncxx::types::b_binary bin_data;
                                bin_data.size = last_chunk_size;
                                bin_data.bytes = pString;
                                bin_data.sub_type = bsoncxx::binary_sub_type::k_binary;

                                bsoncxx::stdx::optional<mongocxx::result::insert_one> result = chunks.insert_one(document{}
                                    << "files_id" << file_id
                                    << "n" << n
                                    << "data" << bin_data
                                    << bsoncxx::builder::stream::finalize);

                                buf.consume(transferred);
                                responseUpload(file_id);
                            }
                            else {
                                do_delete(file_id.to_string());
                            }
                        });
                }
                else {
                    std::string encoded_res = base64_encode(command);
                   
                    const std::uint8_t* pString = (uint8_t*)command.c_str();
                    bsoncxx::types::b_binary bin_data;
                    bin_data.size = last_chunk_size;
                    bin_data.bytes = pString;
                    bin_data.sub_type = bsoncxx::binary_sub_type::k_binary;

                    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = chunks.insert_one(document{}
                        << "files_id" << file_id
                        << "n" << n
                        << "data" << bin_data
                        << bsoncxx::builder::stream::finalize);
               
                    responseUpload(file_id);
                    return;
                }
            }
        }
        catch (...) {
            socket_.close();
        }
    }
    
    
        void do_postfile(long long conlength) {
        auto self(shared_from_this());
        
        read_post_header(conlength);

        }
        void response1(std::string requrl) {
            auto self = shared_from_this();
            mongocxx::options::find opts;

            std::string queryparams = requrl.substr(requrl.find("?") + 1, requrl.length());
            std::string ippparam = queryparams.substr(0, queryparams.find("&"));
            int ipp = std::stoi(ippparam.substr(ippparam.find("=") + 1, ippparam.length()));


            std::string cpparam = queryparams.substr(queryparams.find("&") + 1, queryparams.length());
            int cp = std::stoi(cpparam.substr(cpparam.find("=") + 1, cpparam.length()));

            opts.limit(ipp);
            opts.skip(cp * ipp);


            try {
                auto cursor = coll.find({}, opts);

                bsoncxx::document::view empty_filter;
                int collectionCount = coll.count_documents(empty_filter);



                pt::ptree root;
                pt::ptree children, child;

                int num = 0;

                for (auto&& doc : cursor) {
                    bsoncxx::types::bson_value::view file_id = doc["_id"].get_value();
                    child.put("_id", file_id.get_oid().value.to_string());
                    child.put("length", doc["length"].get_int64().value);
                    child.put("chunkSize", doc["chunkSize"].get_int32().value);
                    child.put("uploadDate", LongToString(doc["uploadDate"].get_date().to_int64()));
                    child.put("filename", doc["filename"].get_utf8().value);
                    child.put("contentType", doc["contentType"].get_utf8().value);

                    children.push_back(std::make_pair("", child));

                }
                root.add_child("files", children);
                root.put("filen", collectionCount);
                root.put("message", "Sending files");

                std::stringstream ret;


                pt::write_json(ret, root);

                std::ostream res_str(&dbuf);

                res_str << "HTTP/1.1 200 OK\r\n"
                    << "\r\n"
                    << ret.str();
                boost::asio::async_write(socket_, dbuf.data(), [this, self](boost::system::error_code ec,
                    std::size_t transferred) {
                        dbuf.consume(transferred);
                        socket_.close();
                    });
            }
            catch (...) {
                socket_.close();
            }
            return;
        }
        void responseUpload(bsoncxx::oid file_id) {
            auto self = shared_from_this();
            pt::ptree root, for_sending;

            std::stringstream ret;
            for_sending.put("message", "File uploaded");

            std::ostringstream oss;

            try {
                bsoncxx::stdx::optional<bsoncxx::document::value> file_result =
                    coll.find_one(document{} << "_id" << file_id << finalize);
                if (file_result) {
                    auto doc = (*file_result).view();
                    root.put("_id", file_id.to_string());
                    root.put("length", doc["length"].get_int64().value);
                    root.put("chunkSize", doc["chunkSize"].get_int32().value);
                    root.put("uploadDate", LongToString(doc["uploadDate"].get_date().to_int64()));
                    root.put("filename", doc["filename"].get_utf8().value);
                    root.put("contentType", doc["contentType"].get_utf8().value);

                    for_sending.put_child("fileInfo", root);
                    pt::write_json(ret, for_sending);
         

                    std::ostream res_str(&dbuf);

                    res_str << "HTTP/1.1 201 Created\r\n"
                        <<"Connection: close\r\n"
                        <<"Content-Type: application/json; charset=utf-8\r\n"
                        <<"Content-Length: "<<ret.str().length()
                        << "\r\n\r\n"
                        << ret.str();

                    try {
                        boost::asio::async_write(socket_, dbuf.data(), [this, self](boost::system::error_code ec,
                            std::size_t transferred) {
                                dbuf.consume(transferred);
                                socket_.close();
                            });
                    }
                    catch (...) {
                        socket_.close();
                    }
                }

            }
            catch (...) {
                socket_.close();
            }
            return;
        }

   
        void do_delete(std::string id) {
            auto self = shared_from_this();
            std::ostream res_str(&dbuf);

            bsoncxx::stdx::optional<bsoncxx::document::value> file = coll.find_one(document{} << "_id" << bsoncxx::oid(id) << finalize);
            if (file) {
                coll.delete_one(document{} << "_id" << bsoncxx::oid(id) << finalize);
                chunks.delete_many(document{} << "files_id" << bsoncxx::oid(id) << finalize);
                res_str << "HTTP/1.1 200 OK\r\n"
                    << "\r\n"
                    << "{\"message\" : \"Successfully removed\"}";
            }
            else {
                res_str << "HTTP/1.1 404 OK\r\n"
                    << "\r\n"
                    << "{\"message\" : \"Not found\"}";
            }
            try {
                boost::asio::async_write(socket_, dbuf.data(), [this, self]
                (boost::system::error_code ec, std::size_t bytes_transferred) {
                        dbuf.consume(bytes_transferred);
                    });
            }
            catch (...) {
                socket_.close();
            }


        }

        void do_download(std::string id, bool preview) {
            auto self = shared_from_this();
            std::ostream resbuf(&dbuf);
            auto d_oid = bsoncxx::oid(id);
            try {
                bsoncxx::stdx::optional<bsoncxx::document::value> file_result =
                    coll.find_one(document{} << "_id" << d_oid << finalize);
                if (file_result) {
                    std::string disposition = "attachment";
                    if (preview) {
                        disposition = "inline";
                    }
                
                    auto filename = (*file_result).view()["filename"].get_utf8().value;
                    auto contentType = (*file_result).view()["contentType"].get_utf8().value;
                    auto conLen = (*file_result).view()["length"].get_int64().value;
                    resbuf << "HTTP/1.1 200 OK\r\n"
                        << "Content-Disposition: "+disposition+"; filename = \"" << filename << "\"\r\n"
                        << "Content-Type: " << contentType << "\r\n"
                        << "Connection: keep-alive\r\n"
                        << "Content-Length: " << conLen << "\r\n"
                        << "Keep-Alive: timeout = 1000\r\n"
                        << "\r\n";
                    try {
                        boost::asio::async_write(socket_, dbuf.data(), [this, self, d_oid]
                        (boost::system::error_code ec,
                            std::size_t bytes_transferred) {
                                dbuf.consume(bytes_transferred);
                                do_download_chunks(d_oid, 0);
                            });
                    }
                    catch (...) {
                        socket_.close();
                    }



                }
                else {
                    resbuf << "HTTP/1.1 404 Not Found\r\n" << "Connection: keep-alive\r\n" << "\r\n"
                        + std::string("{\"message\" : \"Not found\"}\r\n");

                  
                    try {
                        boost::asio::async_write(socket_, dbuf.data(), [this, self](boost::system::error_code ec,
                            std::size_t bytes_transferred) {
                                dbuf.consume(bytes_transferred);
                            });
                    }
                    catch (...) {
                        socket_.close();
                    }

                }
            }
            catch (...) {
                socket_.close();
            }
        }

        void do_download_chunks(bsoncxx::v_noabi::oid d_oid, int n) {
            auto self = shared_from_this();
            std::ostream res_str(&dbuf);
            bsoncxx::stdx::optional<bsoncxx::document::value> chunk_result =
                chunks.find_one(document{} << "files_id" << d_oid << "n" << n << finalize);
            if (chunk_result) {
                std::string json_string = bsoncxx::to_json(*chunk_result);
                std::string data_substr = json_string.substr(json_string.find_last_of("{") + 15);
                data_substr = data_substr.substr(0, data_substr.find("\","));
                std::string decoded = base64_decode(data_substr);
                res_str << decoded;
                try {
                    boost::asio::async_write(socket_, dbuf.data(),
                        [this, self, d_oid, n](boost::system::error_code, std::size_t bytes_transferred)
                        {
                            dbuf.consume(bytes_transferred);
                            do_download_chunks(d_oid, n + 1);
                        });
                }
                catch (...) {
                    socket_.close();
                }
            }
            else {
                socket_.close();
            }
        };

public:
    my_server_session(boost::asio::ip::tcp::socket&& s) : socket_(std::move(s)) {}

    boost::asio::streambuf buf, dbuf;
    

    void start() {
        do_read();
    }
};

class my_server {
public:
    my_server(boost::asio::io_context& io_context, const boost::asio::ip::tcp::endpoint& endpoint)
        : acceptor_(io_context, endpoint), socket_(io_context)
    {
        do_accept();
    }
private:
    void do_accept() {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec) {
                std::make_shared<my_server_session>(std::move(socket_))->start();
                do_accept();
            }

            });
    }
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
};

int main() {

    try {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::endpoint server_endpoint(boost::asio::ip::make_address("127.0.0.1"), 3000);
        my_server s(io_context, server_endpoint);

        io_context.run();

    }
    catch (...) {
        std::cout << "error ocurued";
    }
    return 0;
}


