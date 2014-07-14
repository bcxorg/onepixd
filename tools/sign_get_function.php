<?php
/*****************************************************
** A function, that when called, notifies the onepixd
** that an email message was sent.
*****************************************************/

function sign_get($path, $email_id, $timezone)
{
	/************** Configuration part ********************* */
	$ipnum = "127.0.0.1";	/* IPnumber of the onpixd daemon */
	$port  = "8101";
	$private_key = "/usr/local/keys/key.priv.pem";
	/*********** End Configuration part ******************** */

	if (!isset($path))
		return $result = "Errro: \$path was not set";
	if (!isset($email_id))
		return $result = "Errro: \$email_id was not set";
	if (!isset($timezone))
		return $result = "Errro: \$timezone was not set";

	if (! file_exists($private_key))
		return $result = "Error: $private_key : No such file.";;

	$fp = fopen("$private_key", "r");
	$priv_key = fread($fp, 8192);
	fclose($fp);
	if ($priv_key  === FALSE)
		return $result = "Error: $private_key : read failed.";

	$pkeyid = openssl_get_privatekey($priv_key);
	$now = time();
	$data = "$now" . "," . $email_id" . "," . "$timezone";
	$ret = openssl_sign($data, $signature, $pkeyid, OPENSSL_ALGO_SHA256);
	if ($ret === FALSE)
		return $result = "Error: Signing data failed.";

	$sig = base64_encode($signature);
	$query = "GET /$path/?sent_id=$email_id&sent_tz=$timezone&sig=$sig HTTP/1.1\r\n\r\n";

	$socket = socket_create(AF_UNIX, SOCK_STREAM, 0);

	/* Timeout in 5 seconds */
	$to = array("sec"=>5, "usec"=>0);
	socket_set_option($socket, SOL_SOCKET, SO_SNDTIMEO, $to);
	$ret = socket_connect($socket, $ipnum, $port);
	if ($ret === FALSE)
		return $result = socket_strerror(socket_last_error($socket));

	$ret = socket_write($socket, $query, strlen($query));
	socket_close($socket);
	if ($ret === FALSE)
		return $result = socket_strerror(socket_last_error($socket));

	return $result = "ok";
}
?>
