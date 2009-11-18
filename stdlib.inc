<?php

// if $var is undefined, set it to $default.	otherwise leave it alone
function setDefault ( &$var, $default="" )
{
	if ( !isset($var) ) $var = $default;
}

// if $var is undefined, return $default, otherwise return $var
function getDefault ( $var, $default="" )
{
	return isset($var) ? $var : $default;
}

// takes a URL and returns it without the querystring portion
function urlStripQuery ( $url )
{
	return ($npos=strpos($url,'?')) == false ? $url : substr($url,0,$npos);
}

// returns the URL of the HTTP_REFERER, without the querystring portion
function urlReferer ( )
{
	return urlStripQuery(getenv("HTTP_REFERER"));
}

// returns the URL of the HTTP_REFERER
function urlRefererFull ( )
{
	return getenv("HTTP_REFERER");
}

// returns the name of the current script, without the querystring portion.
// this function is necessary because PHP_SELF and REQUEST_URI and SCRIPT_NAME
// return different things depending on a lot of things like your OS, Web
// server, and the way PHP is compiled (ie. as a CGI, module, ISAPI, etc.)
function scriptName ( )
{
	$me = getenv("SCRIPT_NAME");		// "/path/script"
	if ( empty($me) ) {
	$me = $GLOBALS["PHP_SELF"]; 		// "/path/script/pathinfo"
	if ( empty($me) )
		$me = getenv("REQUEST_URI");	// "/path/script/pathinfo?requeststring..."
	}
	return urlStripQuery($me);
}

// return current fully URL, without "PATH_INFO and REQUEST_STRING"
function urlCurrent ( )
{
	return (getenv("HTTPS")=='on'?'https':'http').'://'.getenv("HTTP_HOST").scriptName();
}

// return current fully URL with "PATH_INFO and REQUEST_STRING"
function urlCurrentFull ( )
{
	return urlCurrent().getenv("PATH_INFO").getenv("QUERY_STRING");
}

// returns true if the referer is the same as the good_referer.  If
// good_refer is not specified, use qualified_me as the good_referer
function matchReferer ( $referer="" )
{
	return (empty($referer)?urlCurrent():$referer) == urlReferer();
}

// redirects to a new URL using meta tags
function redirectToURL ( $url, $message="", $delay=0 )
{
	echo "<meta http-equiv='Refresh' content='$delay; url=$url'>";
	if ( !empty($message) )
	echo "<div style='font-family:Arial,Sans-serif;font-size:12pt;' align=\"center\">$message</div>";
	exit();
}

?>
