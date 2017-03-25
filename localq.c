/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_localq.h"
#include "localq/proto.h"

#include <sys/socket.h>
#include <sys/un.h>

ZEND_DECLARE_MODULE_GLOBALS(localq)

/* True global resources - no need for thread safety here */
static int le_localq;

static int _lq_connect(char *path TSRMLS_DC);
static int _lq_recv_data(void *nil TSRMLS_DC);
static int _lq_send_data(char *data, int data_len, char *method_name, int method_len, char *class_name, int class_len TSRMLS_DC);
static char *_lq_pack_data(char *data, int data_len, char *method_name, int method_len, char *class_name, int class_len, int *total_size TSRMLS_DC);

ZEND_BEGIN_ARG_INFO_EX(push_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, method_name)
	ZEND_ARG_INFO(0, class_name)
ZEND_END_ARG_INFO()

#define SET_ERROR(errno, error) do {	\
	LOCALQ_G(g_errno) = errno;			\
	strcpy(LOCALQ_G(g_errmsg), error);	\
} while(0);

const zend_function_entry localq_functions[] = {
	PHP_ME(Localq, push, push_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Localq, status, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Localq, getError, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
	//PHP_FE_END	/* Must be the last line in localq_functions[] */
};

zend_module_entry localq_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"localq",
	localq_functions,
	PHP_MINIT(localq),
	PHP_MSHUTDOWN(localq),
	PHP_RINIT(localq),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(localq),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(localq),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_LOCALQ_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_LOCALQ
ZEND_GET_MODULE(localq)
#endif

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("localq.sock_path", "", PHP_INI_ALL, OnUpdateString, sock_path, zend_localq_globals, localq_globals)
PHP_INI_END()

static void php_localq_init_globals(zend_localq_globals *localq_globals)
{
	localq_globals->server_fd = -1;
}

PHP_MINIT_FUNCTION(localq)
{
	ZEND_INIT_MODULE_GLOBALS(localq, php_localq_init_globals, NULL);

	REGISTER_INI_ENTRIES();
	
	zend_class_entry Localq;
	INIT_CLASS_ENTRY(Localq, "Localq", localq_functions);
	zend_register_internal_class_ex(&Localq, NULL, NULL TSRMLS_CC);
	LOCALQ_G(server_fd) = -1; // _lq_connect(LOCALQ_G(sock_path));		
	
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(localq)
{
	UNREGISTER_INI_ENTRIES();

	if (LOCALQ_G(server_fd) != -1) {
		close(LOCALQ_G(server_fd));
	}	
	return SUCCESS;
}

PHP_RINIT_FUNCTION(localq)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(localq)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(localq)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "localq support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}


static int _lq_connect(char *path TSRMLS_DC) {
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, path);

	int result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
	if (result == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "connect() failed. %s", strerror(errno));
		return -1;
	}

	return sockfd;
}

static char *_lq_pack_data(char *data, int data_len, char *method_name, int method_len, char *class_name, int class_len, int *total_size TSRMLS_DC) {
	lq_proto_t proto_head = {PROTO_VERSION_1, PROTO_CMD_REQUEST, 0};
	lq_proto_req_t first = {PROTO_REQ_TYPE_HEAD, 0};
	lq_proto_req_t head_data = {PROTO_REQ_TYPE_ARG, data_len};
	lq_proto_req_t head_method = {PROTO_REQ_TYPE_FUNC, method_len};
	lq_proto_req_t head_class = {PROTO_REQ_TYPE_CLASS, class_len};
	int proto_req_size = sizeof(lq_proto_req_t);
	first.length = proto_req_size * 3 + data_len + method_len + class_len;
	proto_head.length = first.length + proto_req_size;

	int head_size = proto_head.length + sizeof(lq_proto_t);
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "head_size[%d] proto_head.length[%d]", head_size, proto_head.length);
	char *header = emalloc(head_size);
	memset(header, 0, head_size);
	int offset = 0;
	memcpy(header, &proto_head, sizeof(lq_proto_t));
	offset = sizeof(lq_proto_t);
	memcpy(header+offset, &first, proto_req_size);
	offset += proto_req_size;
	memcpy(header+offset, &head_method, proto_req_size);
	offset += proto_req_size;
	memcpy(header+offset, method_name, method_len);
	offset += method_len;
	memcpy(header+offset, &head_class, proto_req_size);
	offset += proto_req_size;
	memcpy(header+offset, class_name, class_len);
	offset += class_len;
	memcpy(header+offset, &head_data, proto_req_size);
	offset += proto_req_size;
	memcpy(header+offset, data, data_len);
	
	
	if (total_size) {
		*total_size = head_size;
	}
	return header;
}

static int _lq_send_data(char *data, int data_len, char *method_name, int method_len, char *class_name, int class_len TSRMLS_DC) {
	if (LOCALQ_G(server_fd) == -1) {
		LOCALQ_G(server_fd) = _lq_connect(LOCALQ_G(sock_path) TSRMLS_CC);
	}
	int head_size = 0;
	char *header = _lq_pack_data(data, data_len, method_name, method_len, class_name, class_len, &head_size TSRMLS_CC);
	int send_size = send(LOCALQ_G(server_fd), header, head_size, 0);
	
	lq_proto_t *head = (lq_proto_t *)header;
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "head info, cmd[%d], version[%d], length[%d]\n", head->cmd, head->version, head->length);

	if (send_size == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "send() failed. %s", strerror(errno));
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "send() failed. %s , reconnect...", strerror(errno));
		if ((LOCALQ_G(server_fd) = _lq_connect(LOCALQ_G(sock_path) TSRMLS_CC)) == -1) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "reconnect failed.");
			efree(header);
			return FAILURE;
		}
		
		int try_times = 3;
		while (try_times) {
			--try_times;
			send_size = send(LOCALQ_G(server_fd), header, head_size, 0);
			if (send_size != -1) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "reconnect and resend success. %s", strerror(errno));				
				efree(header);
				return _lq_recv_data(NULL TSRMLS_CC);
			}
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "reconnect and resend failed.");
		efree(header);
		return FAILURE;
	}

	efree(header);
	return _lq_recv_data(NULL TSRMLS_CC);
}

static int _lq_recv_data(void *nil TSRMLS_DC) {
	lq_proto_t head = {0};
	int recv_size = recv(LOCALQ_G(server_fd), &head, sizeof(lq_proto_t), 0);
	if (recv_size == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "recv() failed. %s", strerror(errno));
		return FAILURE;
	}
	if (head.cmd != PROTO_CMD_RESPONSE || head.version != PROTO_VERSION_1 || head.length <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "recv head invalid. head info, cmd[%d], version[%d], length[%d]", head.cmd, head.version, head.length);
		return FAILURE;
	}

	lq_proto_resp_t proto_resp;
	recv_size = recv(LOCALQ_G(server_fd), &proto_resp, sizeof(lq_proto_resp_t), 0);
	if (recv_size == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "recv() failed. %s", strerror(errno));
		return FAILURE;
	}

	if (proto_resp.error_no != 0) {
		int len = strlen(proto_resp.error_msg);
		SET_ERROR(FAILURE, proto_resp.error_msg);
	}

	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "return result, errno[%d], error[%s]\n", proto_resp.error_no, proto_resp.error_msg);
	return proto_resp.error_no == 0 ? SUCCESS : FAILURE;

}

PHP_METHOD(Localq, push) {
	char *data = NULL, *method_name = NULL, *class_name = NULL;
	int data_len = 0, method_len = 0, class_len = 0;
	
	SET_ERROR(SUCCESS, "ok");

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &data, &data_len, &method_name, &method_len, &class_name, &class_len) == FAILURE) {
		SET_ERROR(FAILURE, "invalid params");
		RETURN_FALSE
	}

	int ret = _lq_send_data(data, data_len, method_name, method_len, class_name, class_len TSRMLS_CC);
	if (ret == FAILURE && strcmp(LOCALQ_G(g_errmsg), "ok") == 0) {
		SET_ERROR(FAILURE, "push failed");
	}

	RETURN_LONG(ret)
}

PHP_METHOD(Localq, status) {

}

PHP_METHOD(Localq, getError) {
	if (LOCALQ_G(g_errmsg) == NULL) {
		RETURN_NULL();
	}
	RETURN_STRING(LOCALQ_G(g_errmsg), 1);
}
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
