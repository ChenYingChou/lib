#!/usr/bin/php
<?php
/**
 * php ssl-mail.php
 *      [--host=ssl://smtp.gmail.com]
 *      [--port=465]
 *      [--username={from}]
 *      [--password=!]
 *      [--from=service@{domain}]
 *      [--subject=subject]
 *      [--body="..."]
 *      [--html]
 * recipient-email ...
 */

$domain = @$_SERVER['HOSTNAME'];
$n = strpos($domain, '.');
if ($n === false) {
    $domain = gethostname();
    $n = strpos($domain, '.');
}
if ($n !== false) $domain = substr($domain, $n+1); 

$host = "ssl://smtp.gmail.com";
$port = "465";
$username = "";
$password = "";
$from = "admin@${domain}";
$subject = "";
$to = "";
$html = false;
$body = "";

function help() {
    global $argv, $host, $port, $username, $from, $subject;
    $s = <<<__EOT__
Usage: php {$argv[0]}
    [-h|--host=${host}]
    [-p|--port=${port}]
    [-U|--username=${username}]
    [-P|--password=?]
    [-f|--from=${from}]
    [-s|--subject="${subject}"]
    [-b|--body="..."]
    [-H|--html]
    recipient-email ...
__EOT__;
    exit($s."\n");
}

//============================================================================
require_once 'Console/Getopt.php';

$argv = Console_Getopt::readPHPArgv();
$options = Console_Getopt::getopt($argv, 'b:h:p:U:P:f:s:?',
    ['host=', 'port=', 'username=', 'password=', 'from=', 'subject=',
    'help', 'html', 'body=']);
if (PEAR::isError($options)) exit($options->getMessage()."\n");

$opts = $options[0];
foreach ($opts as $opt) {
    switch($opt[0]) {
        case '--host':
        case 'h':
            $host = $opt[1];
            break;
        case '--port':
        case 'p':
            $port = $opt[1];
            break;
        case '--username':
        case 'U':
            $username = $opt[1];
            break;
        case '--password':
        case 'P':
            $password = $opt[1];
            break;
        case '--from':
        case 'f':
            $from = $opt[1];
            break;
        case '--subject':
        case 's':
            $subject = $opt[1];
            break;
        case '--body':
        case 'b':
            $body = $opt[1];
            break;
        case '--html':
        case 'H':
            $html = true;
            break;
        case '--help':
        case '?':
            help();
    }
}

if (empty($username)) $username = $from;

if (count($options[1]) > 0) {
    $to = trim(implode(', ', $options[1]));
}
if (empty($to)) help();

if (empty($body)) {
    $body = file_get_contents('php://stdin');
} else {
    $body = str_replace('\r', "", $body);
    $body = str_replace('\n', "\r\n", $body);
}

if (trim($body) == '') exit(0);

if (substr($from, -1, 1) != '>') {
    $from = "Service <${from}>";
}
//============================================================================
require_once "Mail.php";

$headers = array (
  'Content-Type' => 'text/plain; charset=utf-8',
  'From' => $from,
  'To' => $to,
  'Subject' => $subject
);

if ($html) {
  require_once "Mail/mime.php";
  $mime = new Mail_mime([
      'head_encoding' => '8bit', // base64, quoted-printable, 7bit, 8bit
      'head_charset'  => 'utf-8',
      'html_encoding' => '8bit',
      'html_charset'  => 'utf-8',
//      'text_encoding' => '8bit',
//      'text_charset'  => 'utf-8'
    ]);
//  $mime->setTxtBody($text, /*true:file*/);
  $mime->setHTMLBody($body);
  $body = $mime->get();
  $headers = $mime->headers($headers);
}

$smtp = Mail::factory('smtp',
  array (
    'host' => $host,
    'port' => $port,
    'auth' => true,
    'username' => $username,
    'password' => $password)
  );

$mail = $smtp->send($to, $headers, $body);

if (PEAR::isError($mail)) {
  echo("\nPEAR.Error> " . $mail->getMessage() . "\n");
} else {
  echo("Message successfully sent!\n");
}

