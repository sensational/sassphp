--TEST--
correctly handles setting and getting source map formats
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
// test default from constructor
$sass->setComments(true);
$sass->setMapPath(__DIR__.'/support/huge.css.map');
list($css, $map) = $sass->compileFile(__DIR__.'/support/huge.scss');
echo $map

?>
--EXPECT_EXTERNAL--
support/huge.css.map
