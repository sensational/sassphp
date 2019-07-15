--TEST--
custom importer: external file
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
$sass->setImporter(function($in){
    echo "$in\n";
    return [null, '.foo { color: red;}'];
});
echo $sass->compile('@import "flupp";');

?>
--EXPECT--
flupp
.foo {
  color: red; }
