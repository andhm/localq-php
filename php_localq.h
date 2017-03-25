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

#ifndef PHP_LOCALQ_H
#define PHP_LOCALQ_H

extern zend_module_entry localq_module_entry;
#define phpext_localq_ptr &localq_module_entry

#define PHP_LOCALQ_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_LOCALQ_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_LOCALQ_API __attribute__ ((visibility("default")))
#else
#	define PHP_LOCALQ_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(localq);
PHP_MSHUTDOWN_FUNCTION(localq);
PHP_RINIT_FUNCTION(localq);
PHP_RSHUTDOWN_FUNCTION(localq);
PHP_MINFO_FUNCTION(localq);


PHP_METHOD(Localq, push);
PHP_METHOD(Localq, status);
PHP_METHOD(Localq, getError);


ZEND_BEGIN_MODULE_GLOBALS(localq)
	int server_fd;
	char *sock_path;
	int g_errno;
	char g_errmsg[32];
ZEND_END_MODULE_GLOBALS(localq)


#ifdef ZTS
#define LOCALQ_G(v) TSRMG(localq_globals_id, zend_localq_globals *, v)
#else
#define LOCALQ_G(v) (localq_globals.v)
#endif

#endif	/* PHP_LOCALQ_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
