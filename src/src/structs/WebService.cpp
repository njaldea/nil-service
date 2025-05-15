#include "WebService.hpp"

namespace nil::service
{
    std::string_view get_route(const WebTransaction& transaction)
    {
        return transaction.request.target();
    }

    void set_content_type(const WebTransaction& transaction, std::string_view type)
    {
        transaction.response.set(boost::beast::http::field::content_type, type);
    }

    void send(const WebTransaction& transaction, std::string_view body)
    {
        transaction.response.result(boost::beast::http::status::ok);
        boost::beast::ostream(transaction.response.body()) << body;
    }

    void send(const WebTransaction& transaction, const std::istream& body)
    {
        transaction.response.result(boost::beast::http::status::ok);
        boost::beast::ostream(transaction.response.body()) << body.rdbuf();
    }
}
