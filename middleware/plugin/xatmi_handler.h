/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#include <http/inbound/caller.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_xatmi_common_declaration.h"

ngx_int_t xatmi_call( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r);
ngx_int_t xatmi_receive( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r);
void xatmi_cancel( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r);
void xatmi_terminate( );
ngx_int_t xatmi_max_service_length();

void error_reporter(ngx_http_request_t* r, ngx_http_xatmi_ctx_t* client_context, ngx_int_t status, const char* message);
void error_handler(ngx_http_request_t* r, ngx_http_xatmi_ctx_t* client_context);

ngx_int_t buffer_size( ngx_int_t value);
ngx_int_t get_header_length(ngx_http_request_t *r);
ngx_int_t copy_headers( ngx_http_request_t *r, header_data_type* headers);
