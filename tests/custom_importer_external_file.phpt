--TEST--
custom importer: external file
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
$sass->setImporter(function($in){
    echo "$in\n";
    return [__DIR__.'/support/foo.scss'];
});
echo $sass->compile('@import "flupp";');

?>
--EXPECT--
flupp
h2 {
  color: green; }
