#pragma once
#include <drogon/HttpFilter.h>

class CorsFilter : public drogon::HttpFilter<CorsFilter> {
public:
    static constexpr bool isAutoCreation = true;
    
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&fcb,
                  drogon::FilterChainCallback &&fccb) override {
        // If it's a preflight OPTIONS request, respond immediately
        if (req->method() == drogon::HttpMethod::Options) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Allow-Credentials", "true");
            resp->setStatusCode(drogon::k200OK);
            fcb(resp);
            return;
        }
        // For other requests, just continue the chain
        fccb();
    }
};
 