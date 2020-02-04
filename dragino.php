<?php
$value=(float)$_REQUEST['value'];
$SET_VALUE=(float)$_REQUEST['SET_VALUE'];
$SET_VALUE2=(float)$_REQUEST['SET_VALUE2'];
$power=(float)$_REQUEST['power'];
$CONSTRAIN=(float)$_REQUEST['CONSTRAIN'];
$K_P=(float)$_REQUEST['K_P'];
$K_I=(float)$_REQUEST['K_I'];
$K_D=(float)$_REQUEST['K_D'];
$DIFF_SUM=(float)$_REQUEST['DIFF_SUM'];

Define('DB_HOST', 'localhost');
Define('DB_NAME', '');
Define('DB_USER', '');
Define('DB_PASSWORD', '');

$link = mysql_connect(DB_HOST, DB_USER, DB_PASSWORD);
if (!$link) return false;
if(!mysql_select_db(DB_NAME, $link))return false;
mysql_query("INSERT INTO pid (value,SET_VALUE,power,CONSTRAIN,K_P,K_I,K_D,SET_VALUE2,DIFF_SUM) values({$value},{$SET_VALUE},{$power},{$CONSTRAIN},{$K_P},{$K_I},{$K_D},{$SET_VALUE2},{$DIFF_SUM});");
echo "OK";
mysql_close($link);
?>