--TEST--
Making sure ARG_INFO is present
--SKIPIF--
<?php if (!extension_loaded("sass") || !class_exists('ReflectionClass')) print "skip"; ?>
--FILE--
<?php

$c = new ReflectionClass("Sass");
$m = $c->getMethod("setIncludePath");
$params = $m->getParameters();
echo $params[0]."\n";

$m = $c->getMethod("setStyle");
$params = $m->getParameters();
echo $params[0]."\n";
?>
--EXPECT--
Parameter #0 [ <required> $include_path ]
Parameter #0 [ <required> $style ]
