--TEST--
compiles correct Sass file and provides source embedding
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
$sass->setEmbed(true);
$css = $sass->compileFile(__DIR__.'/support/test.scss');
echo $css;


?>
--EXPECT--
@import url(../blahblah/blah.blah);
div {
  blah: "hello 4 world px bloo blah"; }

div {
  blah: "foo iphone"; }