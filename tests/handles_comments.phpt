--TEST--
correctly handles setting and getting comments
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
// test default from constructor
echo $sass->getComments();

// test SASS_SOURCE_NONE
$sass->setComments(false);
echo $sass->getComments();

// test SASS_SOURCE_DEFAULT
$sass->setComments(true);
echo $sass->getComments();


?>
--EXPECT--
001
