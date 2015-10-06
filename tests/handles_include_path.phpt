--TEST--
correctly handles includePath
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
$sass->setIncludePath(__DIR__.'/support');
$css = $sass->compile('
@import "foo";
');

echo $css;
echo $sass->compileFile(__DIR__.'/support/import.scss');

?>
--EXPECT--
h2 {
  color: green; }
h2 {
  color: green; }

h1 {
  color: red; }
