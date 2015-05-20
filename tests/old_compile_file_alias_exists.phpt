--TEST--
old compile_file() alias exists
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
$css = $sass->compile_file('tests/support/test.scss');
echo $css;

?>
--EXPECT--
@import url(../blahblah/blah.blah);
div {
  blah: "hello 4 world px bloo blah"; }

div {
  blah: "foo iphone"; }
