<?php
// PHP 에러 코드 확인
ini_set('display_errors', 'On');
error_reporting(E_ALL | E_STRICT);

// MySQL 접속
$mysql_host = '192.168.0.3';
$mysql_user = 'test';
$mysql_password = '1234';
$mysql_db = 'pet';
$conn = mysqli_connect($mysql_host, $mysql_user, $mysql_password,$mysql_db);

// DB에서 원하는 데이터 검색
$sql = $_POST['str'];

$result = mysqli_query($conn,$sql);
//$row = mysqli_fetch_array($result);
//echo $row['date'];
$return_array = array();
while ($row = mysqli_fetch_array($result, MYSQL_ASSOC))
{
      //$row_array['date'] = $row['date'];
      $var = $row['time'];
      $datetime = strtotime($var);
      $row_array['date']= $datetime*1000;// delete milisecond
      $row_array['temperature'] = $row['temp'];
      $row_array['humidity']= $row['humid'];
      $row_array['water'] = $row['water'];
      array_push($return_array, $row_array);
}
//$datetimeUTC = (strtotime($var)*1000);
//echo $datetimeUTC;
$result_arr = json_encode($return_array);
mysqli_free_result($result);
mysqli_close($conn);
echo $result_arr;
?>
