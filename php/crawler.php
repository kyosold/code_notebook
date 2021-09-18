<?php

define('CMD_FILE', "./cmd.txt");
$ext = "htm";

if ($argc != 4) {
    echo "Usage: php {$argv[0]} [url] [begin,end,fmt] [dir]\n";
    echo "Example:\n";
    echo "  php {$argv[0]} 'https://xbookcn.net/book/tiandi' '1,200' './小说/天地/'";
    echo "Params:\n";
    echo "begin,end,fmt: 1,102,%03d => 001,002...102\n";
    die();
}

$url = $argv[1];
$chapter = $argv[2];
$save_dir = $argv[3];

$is_fmt = false;
$tmp = explode(',', $chapter);
$cb = $tmp[0];
$ce = $tmp[1];
$fmt = '';
if (count($tmp) == 3) {
    $is_fmt = true;
    $fmt = $tmp[2];
}

// if (substr($url, -1) == '/')
//     $url = substr($url, 0, -1);

check_dir($save_dir);
log_cmd($argc, $argv);

$proxy = array(
    'http' => array(
        'proxy' => 'tcp://127.0.0.1:1087',
        'request_fulluri' => true,
    ),
);
$cx = stream_context_create($proxy);

$i = 0;
for ($i = $cb; $i <= $ce; $i++) {

    if ($is_fmt)
        // $uri = $url . "/" . sprintf($fmt, $i) . "." . $ext;
        $uri = $url  . sprintf($fmt, $i) . "." . $ext;
    else
        // $uri = $url . "/" . $i . "." . $ext;
        $uri = $url  . $i . "." . $ext;

    $save_file = $save_dir . "/" . $i . "." . $ext;

    $txt = file_get_contents($uri, FALSE, $cx);
    if ($txt === FALSE)
        die();

    file_put_contents($save_file, $txt);

    $percent = $i / $ce * 100;

    echo floor($percent) . "% " . $uri . " Finished.\n";
}

// 检查文件夹是否存在，不存在则创建
function check_dir($dir)
{
    if (!file_exists($dir))
        mkdir($dir);
}

// 写命令到日志中
function log_cmd($argc, $argv)
{
    $cmd_str = "php " . $argv[0];
    $i = 0;
    for ($i = 1; $i < $argc; $i++) {
        $cmd_str .= " '" . $argv[$i] . "'";
    }
    $cmd_str .= "\n";

    if (!$fp = fopen(CMD_FILE, 'a+')) {
        echo "Can't open file " . CMD_FILE . "\n";
        die();
    }
    if (fwrite($fp, $cmd_str) === FALSE) {
        echo "Can't write cmd({$cmd_str}) to file(" . CMD_FILE . ")\n";
        die();
    }
    fclose($fp);
}
