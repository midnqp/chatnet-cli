<?php 
/* 	MESSAGE
To debug or to have PHP print stuffs in the console, set $post to 
your desired variable. And run: `$ php net.php`. This is the simplest easiest way to do it.


sudo chown -R www-data:www-data /var/www/html/
# Might crash the server, if it's already running.
# And may continuously display the last msg, if someone was doing --read_AllMsg.
*/

$Server = new Server();
$Server->dir = './cache';
$Server->init();








function str_begins_with($string, $substr) {
	if (substr($string, 0, strlen($substr)) === $substr)
		return true;
	else return false;
}
function read_uSend() {
	global $post;
	$pArr = explode(" ", $post);
	return $pArr[1];
}
function ActiveList_uSend($what) {
	$uSend = read_uSend();
	$_uSend_ = "[+] ".$uSend."\n";
	global $activeFn;
	$active = file_get_contents($activeFn);
	if ($what === "add") {
		if (strpos($active, $_uSend_) === false) {
			file_put_contents($activeFn, $data=$_uSend_, $flags=FILE_APPEND);
		}
		echo file_get_contents($activeFn);
	}


	else if ($what === "remove") {
		if (strpos("$active", "$_uSend_") !== false) {
			// his name already exists. Needs to be removed.
			$active = str_replace($_uSend_, '', $active);
			file_put_contents($activeFn, $active);
			echo file_get_contents($activeFn);
		}
	}
}
function read_chatroomFn() {
	global $post, $Server;
	$pArr = explode(" ", $post);
	return $Server->dir."/{$pArr[2]}";
}
function read_newSharedKey() {
	$randByte = random_bytes(20);
	date_default_timezone_set("Asia/Dhaka");
	$now = date("ymdHis");

	$key = bin2hex($randByte);
	return "$now-$key";
}




$post = file_get_contents('php://input');
#$post = "--write_exit someone";
#$post = "--read_AllMsg someone";
#$post = "--write_chatroom midnqp someone";
#$post = "--read_active someone";
#$post = "--write_ThisMsg uSend midnqp-adf How are you Muhammad?";


if (str_begins_with($post, '--read_active')) {
	ActiveList_uSend("add");
}


else if (str_begins_with($post, '--read_AllMsg')) {
	$uSend = read_uSend();
	$msgAll = "";
	
	$uSendFnAll = glob("$Server->dir/$uSend-*");
	foreach ($uSendFnAll as $uSendFn) {
		$msgAll .= file_get_contents($uSendFn);
		file_put_contents($uSendFn, ""); //emptying files I've read
	}
	if ($msgAll !== "") {
		// yes, new msg
		echo $msgAll;
		exit();
	}
	else exit("");
}


else if (str_begins_with($post, "--write_chatroom")) {
	$uSend = read_uSend();
	$uRecv = explode(" ", $post)[2];
	
	//RecvAllChatrooms = glob("$Server->dir/$uRecv-*");
	$newKey = read_newSharedKey();
	$chatroomFn = "$uRecv-$newKey";
	file_put_contents("$Server->dir/$chatroomFn", "");
	exit($chatroomFn);
}


else if (str_begins_with($post, "--write_ThisMsg")) {
	$uSend = read_uSend();
	$chatroomFn = read_chatroomFn();
	
	$postExplode = explode(" ", $post);
	$msgText = "[$uSend] ";
	for ($i = 3; $i < count($postExplode); $i++) $msgText .= $postExplode[$i] . " ";
	$msgText = substr($msgText, 0, strlen($msgText)-1); //last space due to for-loop
	
	file_put_contents($chatroomFn, $data=$msgText, $flags=FILE_APPEND);
}


else if (str_begins_with($post, "--write_exit")){
	ActiveList_uSend("remove");
}








class Server {
	public $dir;

	function init() {
		$activeFn = $this->dir . '/_active_';
		
		if (! file_exists($this->dir)) mkdir($this->dir);
		if (! file_exists($activeFn)) file_put_contents($activeFn, '');
		
		$GLOBALS['activeFn'] = $activeFn;
	}
}
?>
