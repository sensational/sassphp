--TEST--
set output_path when compiling in memory
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
echo $sass->compile('@import "subsub/a.scss";', __DIR__.'/support/sub/a.scss');

?>
--EXPECT--
body {
  background: red; }
