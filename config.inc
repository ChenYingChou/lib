<?php
// vim: set ts=4 sw=4:

/*
 * Global Constants and Variables
 */
if (!defined('_ERROR_EXCEPTION_'))
	  define('_ERROR_EXCEPTION_',true);
if (!defined('_DEBUG_LEVEL_'))
	  define('_DEBUG_LEVEL_',0);
if (!defined('_LOG_UP_LEVEL_'))
	  define('_LOG_UP_LEVEL_',PEAR_LOG_WARNING);  // EMERG|ALERT|CRIT|ERR|WARNING|NOTICE|INFO|DEBUG)

if (!defined('CLASS_PATH'))
	  define('CLASS_PATH','/www/classes/');
if (!defined('DATA_PATH'))
	  define('DATA_PATH','/www/log/');			  // CacheObj 用 (Database Cache)
if (!defined('LOG_PATH'))
	  define('LOG_PATH','/www/log/');			  // 日誌用
if (!defined('CACHE_PATH'))
	  define('CACHE_PATH','/www/img_cache/');	  // 縮圖用
if (!defined('IMAGE_PATH'))
	  define('IMAGE_PATH','/www/images/');		  // 圖檔: Icon 等小圖檔案路徑
if (!defined('IMAGE_URL'))
	  define('IMAGE_URL','/images/');			  // 圖檔: Icon 等小圖 URL 位置

if (!defined('SYS_NAME'))
	  define('SYS_NAME','');
if (!defined('SYS_CODEPAGE'))
	  define('SYS_CODEPAGE','UTF8');
if (!defined('DBSVR'))
	  define('DBSVR','dbtype://user:passwd@host/db');  // !!若使用資料庫一定要定義!!
if (!defined('OS_CODEPAGE')) {
  if (PHP_OS == 'WINNT')
	  define('OS_CODEPAGE','BIG5'); 			  // BIG5/CP950, GBK/CP936
  else
	  define('OS_CODEPAGE',SYS_CODEPAGE);
}

/* 給 site.inc 注意事項
  // common.inc 在 include 本檔案之前已定義 $_db_options_ 如下:
  $_db_options_ = array(...);

  // 若要在此變更或新增資料庫選項, 修改範例如下:
  // $_db_options_['persistent'] = true;
  // $_db_options_['portability'] = DB_PORTABILITY_LOWERCASE | DB_PORTABILITY_RTRIM;
*/

function _exceptions_error_handler_($errno, $errstr, $errfile, $errline) {
	throw new ErrorException($errstr,0,$errno,$errfile,$errline);
}

function _config_initialize_() {
	// set class autoload environment
	if (CLASS_PATH != '') {
		$cpath = realpath(CLASS_PATH);
		$IncPath = get_include_path();
		if (!in_array($cpath,explode(PATH_SEPARATOR,$IncPath)))
			set_include_path($cpath.PATH_SEPARATOR.$IncPath);
		$GLOBALS['_CLASSPATH'][] = $cpath.DIRECTORY_SEPARATOR;
	}

	// set database environment
	global $_DBSVR;
	$_DBSVR = (empty($_DBUSR) || !isset($_DBPWD))
			? DBSVR : preg_replace('|://(.*?):(.*?)@|',"://$_DBUSR:$_DBPWD@",DBSVR);
	if (PHP_OS == 'Linux' || PHP_OS == 'Unix') {
		// db2 for unixODBC
		if (strncasecmp($_DBSVR,'odbc:',5) == 0) {
			if (!empty($_DBUSR)) {
				putenv("DB2INSTANCE=$_DBUSR");
			} else {
				putenv('DB2INSTANCE=db2admin');
			}
		}
	}

	// for logging
	global $_SYSLOG;
	$_SYSLOG = LOG_PATH;
	if (!is_dir($_SYSLOG)) $_SYSLOG = '';
	$_SYSLOG .= SYS_NAME.'.log';

	if (function_exists('mb_internal_encoding')) mb_internal_encoding(SYS_CODEPAGE);

	set_error_handler('_exceptions_error_handler_',E_USER_ERROR);
}
_config_initialize_();

?>
