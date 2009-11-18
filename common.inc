<?php
// vim: set ts=4 sw=4:

$_db_options_ = array(
					//'portability' => DB_PORTABILITY_LOWERCASE | DB_PORTABILITY_RTRIM,
					//'persistent'	=> true
				);

require_once 'Log.php'; 		// pear/Log
require_once 'DB.php';			// pear/DB
require_once 'site.inc';
require_once 'config.inc';
if (SYS_NAME != '') @include_once SYS_NAME.'.inc';
if (!isset($_SERVER['REMOTE_ADDR'])) $_SERVER['REMOTE_ADDR'] = '127.0.0.1';

/*
 * Global Functions
 */
function __autoload($className) {
	$bt = debug_backtrace();
	if (@$bt[1]['function'] == 'class_exists') return;

	global	$_CLASSPATH;
	foreach ($_CLASSPATH as $path) {
		$file = $path.$className.'.inc';
		if (file_exists($file)) {
			require_once $file;
			return;
		}
	}
	$s = "Missing class: $className.";
	return eval("class $className{" .
				' function __construct(){' .
				"  _ERROR('$s');" .
				"  throw new Exception('$s');" .
				' }' .
				'}');
}

function Logger() {
	static $logger;
	if (!isset($logger)) {
		global $_SYSLOG;
		$conf = array('mode' => 0664, 'timeFormat' => '%Y-%m-%d %H:%M:%S');
		$logger = Log::singleton('file',$_SYSLOG,SYS_NAME,$conf,_LOG_UP_LEVEL_);
	}
	return $logger;
}
function _logmsg_($msg) { return '['.$_SERVER['REMOTE_ADDR'].'] '.trim($msg); }
function logLevel($mask)  { Logger()->setMask($mask); }
function logAlert($msg)   { Logger()->alert(_logmsg_($msg)); }
function logCritical($msg){ Logger()->crit(_logmsg_($msg)); }
function logError($msg)   { Logger()->err(_logmsg_($msg)); }
function logWarning($msg) { Logger()->warning(_logmsg_($msg)); }
function logNotice($msg)  { Logger()->notice(_logmsg_($msg)); }
function logInfo($msg)	  { Logger()->info(_logmsg_($msg)); }
function logDebug($msg, $reckonTime=false) {
	static $t;
	if ($reckonTime) {
		$tt = $t;
		$t = microtime(true);
		if (is_float($tt)) $msg = sprintf('elapsed %.3f ... ',$t-$tt).$msg;
	}
	Logger()->debug(_logmsg_($msg));
}

/** 印出程式除錯追蹤點
 * @param  bool $return=true	是否以字串方式返回, 否則直接輸出到 stdout
 * @return mixed 				$return==true 時返回要輸出追蹤的字串, 否則返回 true
 */
function _BACKTRACE($return=true) {
	if ($return) ob_start();
	$backtrace = debug_backtrace();
	array_shift($backtrace);
	$bt = $backtrace[0];
	echo ">>> Stack trace at {$bt['file']}({$bt['line']})\n";
	array_shift($backtrace);

	$num = 0;
	foreach ($backtrace as $bt) {
		$args = '';
		foreach ($bt['args'] as $a) {
			if (!empty($args)) $args .= ', ';
			switch (gettype($a)) {
				case 'integer':
				case 'double':
					$args .= $a;
					break;
				case 'string':
					$args .= "'$a'";
					break;
				case 'array':
					$args .= 'Array('.count($a).')';
					break;
				case 'object':
					$args .= 'Object('.get_class($a).')';
					break;
				case 'resource':
					$args .= 'Resource('.strstr($a, '#').')';
					break;
				case 'boolean':
					$args .= $a ? 'True' : 'False';
					break;
				case 'NULL':
					$args .= 'Null';
					break;
				default:
					$args .= 'Unknown';
			}
		}

		$className = @$bt['class'];
		$type = @$bt['type'];
		$function = @$bt['function'];
		$caller = $className == $function || $function == '__construct'
				? "new $className" : "{$className}{$type}{$function}";

		$num++;
		echo @" #$num {$bt['file']}({$bt['line']}): {$caller}($args)\n";
	}
	if ($return) {
		$rs = ob_get_contents();
		ob_end_clean();
		return $rs;
	}
	return true;
}

function isDebug($level=null) {
	if (empty($level))
		return(logger()->getMask() & Log::MASK(PEAR_LOG_DEBUG));
	return _DEBUG_LEVEL_ >= $level;
}

function _ERROR($errMsg, $errObj=null) {
	if (isDebug(2)) {
		global	$_ERROBJ;
		$_ERROBJ = empty($errObj) ? $errMsg : $errObj;
	}
	logError($errMsg);
	if (isDebug()) logDebug(trim(_BACKTRACE()));
	if (_ERROR_EXCEPTION_) throw new Exception($errMsg);
	echo "<br/><p style=\"color:#f00\"><b>Error: </b>$errMsg</p><br/>\n";
}

function DBError($rs) {
	if (DB::isError($rs)) {
		if (isDebug(9)) logDebug(print_r($rs,true));
		if ($rs->message == 'DB Error: invalid' && isset($rs->userinfo)) {
			$s = $rs->userinfo;
			$rs->message = preg_match('/\[nativecode=ERROR:\s*(.*)\]\s*$/',$s,$m)
							? $m[1] : $s;
		}
		return true;
	}
	return false;
}

function DB($svr='', $encoding=SYS_CODEPAGE) {
	static $db;
	if (!isset($db)) {
		if (empty($svr)) {
			global $_DBSVR;
			$svr = $_DBSVR;
		}
		global $_db_options_;
		$db = DB::connect($svr,$_db_options_);
		if (DBError($db)) {
			// Remove username/password, Ex:
			// 'mysql://username:password@host/db' -> 'mysql://***:***@host/db'
			$s = preg_replace('|://.+@|','://***:***@',$svr);
			_ERROR("DB::connect('$s') -> ".$db->getMessage(),$db);
			return null;
		}
		if ($db->phptype == 'pgsql') {
			$rc = $db->connection;		 // resource connection of pgsql
			if (!empty($encoding) && strcasecmp(pg_client_encoding($rc),$encoding) != 0 )
				pg_set_client_encoding($rc,$encoding);
		} else if ($db->phptype == 'mysql') {
			if (!empty($encoding)) $db->query("set names '$encoding'");
		} else if ($db->phptype == 'odbc') {
			$db->setOption('portability',DB_PORTABILITY_LOWERCASE | DB_PORTABILITY_RTRIM);
		}
	}
	return $db;
}

function transaction_start() {
	DB()->autoCommit(false);
	if (isDebug(6)) logDebug('Transaction.Start');
}

function transaction_commit() {
	$db = DB();
	$rs = $db->commit();
	$db->autoCommit(true);
	if ( DBError($rs) ) {
		_ERROR('SQL(Commit) -> '.$rs->getMessage(),$rs);
		return null;
	}
	if (isDebug(6)) logDebug('Transaction.Commit');
	return $rs;
}

function transaction_rollback() {
	$db = DB();
	$rs = $db->rollback();
	$db->autoCommit(true);
	if ( DBError($rs) ) {
		_ERROR('SQL(Rollback) -> '.$rs->getMessage(),$rs);
		return null;
	}
	if (isDebug(6)) logDebug('Transaction.Rollback');
	return $rs;
}

function getQueryS($SQL, $params=null) {
	$db = DB();
	$rs = $db->getOne($SQL,$params);
	if ( DBError($rs) ) {
		_ERROR("SQL('{$db->last_query}') -> ".$rs->getMessage(),$rs);
		return null;
	}
	if (isDebug(6)) logDebug("getQueryS('{$db->last_query}')=$rs");
	return $rs;
}

function getQueryI($SQL, $params=null) {
	$rs = getQueryS($SQL,$params);
	return is_null($rs) ? null : (int)$rs;
}

function &getQueryV($SQL, $params=null, $mode=DB_FETCHMODE_ASSOC) {
	$db = DB();
	$rs = $db->getAll($SQL,$params,$mode);
	if ( DBError($rs) ) {
		_ERROR("SQL('{$db->last_query}') -> ".$rs->getMessage(),$rs);
		return null;
	}
	if (isDebug(6)) logDebug("getQueryV('{$db->last_query}')=\n".print_r($rs,true));
	return $rs;
}

function getQueryMap($SQL, $params=null, $mode=DB_FETCHMODE_ASSOC, $group=false) {
	$db = DB();
	$rs = $db->getAssoc($SQL,false,$params,$mode,$group);
	if ( DBError($rs) ) {
		_ERROR("SQL('{$db->last_query}') -> ".$rs->getMessage(),$rs);
		return null;
	}
	if (isDebug(6)) logDebug("getQueryMap('{$db->last_query}')=\n".print_r($rs,true));
	return $rs;
}

function getQueryRow($SQL, $params=null, $mode=DB_FETCHMODE_ASSOC) {
	$db = DB();
	$rs = $db->getRow($SQL,$params,$mode);
	if ( DBError($rs) ) {
		_ERROR("SQL('{$db->last_query}') -> ".$rs->getMessage(),$rs);
		return null;
	}
	if (isDebug(6)) logDebug("getQueryRow('{$db->last_query}')=\n".print_r($rs,true));
	return $rs;
}

function getQueryCol($SQL, $params=null, $col=0) {
	$db = DB();
	$rs = $db->getCol($SQL,$col,$params);
	if ( DBError($rs) ) {
		_ERROR("SQL('{$db->last_query}') -> ".$rs->getMessage(),$rs);
		return null;
	}
	if (isDebug(6)) logDebug("getQueryCol('{$db->last_query}')=\n".print_r($rs,true));
	return $rs;
}

function execSQL($SQL, $param=null) {
	$db = DB();
	$rs = $db->query($SQL,$param);
	if ( DBError($rs) ) {
		_ERROR("SQL('{$db->last_query}') -> ".$rs->getMessage(),$rs);
		return null;
	}
	$rs = $db->affectedRows();
	if (isDebug(6)) logDebug("execSQL('{$db->last_query}')=".$rs);
	return $rs;
}

/**
 * @param mixed $value			string or string-array
 * @return mixed				string or string-array with quoted by DB()
 */
function Quote($value) {
	if (is_array($value)) {
		$result = array();
		foreach ($value as $v) {
			$result[] = DB()->quoteSmart($v);
		}
		return $result;
	}
	return DB()->quoteSmart($value);
}

/** 分析 PostgreSQL 資料庫的陣列欄位, 返回陣列
 * @param string $value			array-string of PostgreSQL: '{item1,item2,...}'
 * @return array(item1,item2,...)
 */
function parseArrayField($value) {
	$value = trim($value);
	if (@substr($value,0,1) != '{' || substr($value,-1,1) != '}')
		_ERROR('array value must be "{...}": '.$value);

	$rs = array();
	$size = strlen($value)-1;  // not include last char('}')
	$n1 = 0;
	while (++$n1 < $size) {
		while ($n1 < $size && ($c=substr($value,$n1,1)) <= ' ') $n1++;
		if ($n1 >= $size) break;
		if ($c == ',') _ERROR('malformed array value: '.$value);

		$n2 = $n1;
		$n0 = 0;
		if ($c == '"') {
			while (++$n2 < $size && ($c=substr($value,$n2,1)) != '"') {
				if ($c == "\\") $n0 = $n2++;
			}
			if ($c != '"') _ERROR('unmatch double quoted: '.$value);
			$sym = substr($value,$n1+1,$n2-$n1-1);
			if ($n0 > 0) $sym = eval("return \"$sym\";");
		} else {
			while (++$n2 < $size && ($c=substr($value,$n2,1)) > ' ' && $c != ',') {
				if ($c == "\\") $n0 = $n2++;
			}
			$sym = substr($value,$n1,$n2-$n1);
			if ($n0 > 0) $sym = eval("return \"$sym\";");
			if ($sym == 'NULL') $sym = null;
		}

		// 允許 recursive array
		$rs[] = @(substr($sym,0,1) == '{' || substr($sym,-1,1) == '}')
			  ? parseArrayField($sym) : $sym;
		$n1 = $n2;
		while ($n1 < $size && ($c=substr($value,$n1,1)) != ',') $n1++;
	}
	return $rs;
}

/** 將陣列組合成 PostgreSQL 資料庫的陣列欄位格式
 * @param array $value			array(item1,item2,...)
 * @return string				array-string of PostgreSQL: '{item1,item2,...}'
 */
function makeArrayField($input, $removeEmpty=false) {
	$rs = '';
	foreach ($input as $val) {
		if ($removeEmpty && empty($val)) continue;
		if (is_null($val)) {
			$val = 'NULL';
		} else if (is_array($val)) {	// 允許 recursive array
			$val = makeArrayField($val,$removeEmpty);
		} else if (strtoupper($val) == 'NULL') {
			$val = '"NULL"';
		} else if (strpbrk($val," ,{}\t\"\\") !== false) {
			$val = '"'.addcslashes($val,"\"\\").'"';
		}
		$rs .= ",$val";
	}
	return $rs === '' ? null : '{'.substr($rs,1).'}';
}

/** 逗號分隔的字串轉成陣列
 * @param mixed $input			array or comma delimited string
 * @return array
 */
function strToArray($input) {
	return is_array($input) ? $input : preg_split('/\s*,\s*/',trim($input," \t\r\n\f"));
}

/** 逗號分隔的字串轉成陣列, 並將所有值轉成小寫
 * @param mixed $input			array or comma delimited string
 * @return array
 */
function strToArrayLowerCase($input) {
	if (is_array($input)) {
		return array_map('mb_strtolower',$input);
	}
	return is_null($input) || $input === '' ? array()
			: preg_split('/\s*,\s*/',mb_strtolower(trim($input," \t\r\n\f")));
}

/**  將 JSON.stringify 轉成 PHP 關連陣列
 * @param mixed $src
 * @param optional Array $args
 * @return Array
 */
function jsonToArray($src, $args=null) {
	if (is_null($src)) return null;
	if (!is_null($args) && is_string($src)) {
		if (!isset($args[$src])) return null;
		$src = $args[$src];
	}
	return is_string($src) ? json_decode($src,true) : (array)$src;
}

/** 陣列轉成逗號分隔的字串
 * @param array $input
 * @return string				comma delimited string
 */
function arrayToCommaStr($input) {
	return is_array($input) ? implode(',',$input) : (string)$input;
}

/** 將陣列/群組欄位轉成適合 HTML 輸出格式
 * @param array $input			一層(group)或多層(array of group)
 * @return string				含有適合 HTML 輸出的字串
 */
function arrayToHtmlStr($input) {
	if (!is_array($input)) return $input;
	$rs = '';
	if (is_array(reset($input))) {
		foreach ($input as $v) {
			$s = arrayToHtmlStr($v);
			if (!empty($s)) {
				if ($rs != '') $rs .= '<hr />';
				$rs .= $s;
			}
		}
	} else {
		foreach ($input as $k => $v) {
			if ($v != '') {
				if ($rs != '') $rs .= '<br />';
				$rs .= "{$k}:$v";
			}
		}
	}
	return $rs;
}

/** 將陣列/群組欄位轉成適合文字輸出格式
 * @param array $input			一層(group)或多層(array of group)
 * @return string				含有適合 HTML 輸出的字串
 */
function arrayToText($input) {
	if (!is_array($input)) return $input;
	$rs = '';
	if (is_array(reset($input))) {
		foreach ($input as $v) {
			$s = arrayToText($v);
			if (!empty($s)) {
				if ($rs != '') $rs .= "\r\n----\r\n";
				$rs .= $s;
			}
		}
	} else {
		foreach ($input as $k => $v) {
			if ($v != '') {
				if ($rs != '') $rs .= "\r\n";
				$rs .= "{$k}:$v";
			}
		}
	}
	return $rs;
}

/** 返回 GMT 時間格式
 * @param string $time
 * @return string
 */
function GMTTimestamp($time) {
	return $mtime = gmdate("D, d M Y H:i:s",$time)." GMT";
}

/** 返回是否由 LAN 端連線
 * 127. *  /  8 -> 0x7F00
 *  10. *  /  8 -> 0x0A00
 * 172.16  / 12 -> 0xAC10
 * 192.168 / 16 -> 0xC0A8
 * @return bool
 */
function isLAN() {
	//return preg_match("/^(127\.|10\.|172\.(1[6-7]|2[0-9]|3[0-1])\.|192\.168\.)/",$_SERVER["REMOTE_ADDR"]);
	$ips = explode('.', $_SERVER['REMOTE_ADDR']);
	$N = ($ips[0] * 256) + $ips[1];
	return $N == 0xC0A8 || ($N & 0xFFF0) == 0xAC10 || ($N & 0xFF00) == 0x0A00 ||
			($N & 0xFF00) == 0x7F00;
}

/** 返回 CodePage 物件
 * @return object CodePage
 */
function CodePage() {
	static $cp;
	if (!isset($cp)) $cp = CodePage::Create(OS_CODEPAGE,SYS_CODEPAGE);
	return $cp;
}

/** 根據本AP系統編碼(SYS_CODEPAGE)將字串轉成作業系統編碼(OS_CODEPAGE)
 * @param string $str
 * @return string
 */
function Sys2OS($str) {
	return CodePage()->Sys2OS($str);
}

/** 根據作業系統編碼(OS_CODEPAGE)將字串轉成本AP系統編碼(SYS_CODEPAGE)
 * @param string $str
 * @return string
 */
function OS2Sys($str) {
	return CodePage()->OS2Sys($str);
}

/** 將 record(hash map) 的值轉換成適合網頁輸出
 * @param Array $row
 * @return Array $row
 */
function recordToHTML($row) {
	foreach ($row as $key => $val) {
		if (is_array($val)) {
			$row[$key] = recordToHTML($val);
		} else {
			$row[$key] = htmlspecialchars($val);
		}
	}
	return $row;
}

/** 檢查參數資料中是否含有  "<script" 或 "<iframe" 標籤
 * @param Array $args	要檢查的參數陣列, 通常為 $_REQUEST
 * @return boolean		返回是否含有 "<script" 或 "<iframe" 標籤
 */
function findJSAttack($args) {
	foreach ($args as $v) {
		if (is_string($v)) {
			$s = strtolower($v);
			if (strpos($s,'<script') !== false || strpos($s,'<iframe') !== false)
				return true;
		} else if (is_array($v)) {
			if (findJSAttack($v)) return true;
		}
	}
	return false;
}
function checkJSAttack($msg, $args=null) {
	if (empty($args)) $args = $_REQUEST;
	if (findJSAttack($args)) {
		logAlert("Javascript Attack in {$_SERVER['SCRIPT_NAME']}: args=".print_r($args,true));
		echo $msg;
		exit;
	}
}

function genParamAG($v) {
	return empty($v) || $v === '{}' ? null : $v;
}
function genParamBoolean($v) {
	if (is_string($v)) {
		return strpos('1TtYy',substr($v,0,1)) !== false ? '1' : '0';
	}
	return (bool)$v ? '1' : '0';
}
function genParamDate($v) {
	return empty($v) ? null : $v;
}
function genParamNumber($v) {
	return is_numeric($v) ? $v : null;
}
function genParamString($v) {
	if (is_string($v)) return $v;
	if (is_array($v) || is_object($v)) return json_encode($v);
	return strval($v);
}

?>
