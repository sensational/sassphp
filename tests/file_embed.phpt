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

/*# sourceMappingURL=data:application/json;base64,ewoJInZlcnNpb24iOiAzLAoJImZpbGUiOiAidGVzdHMvc3VwcG9ydC90ZXN0LmNzcyIsCgkic291cmNlcyI6IFsKCQkidGVzdHMvc3VwcG9ydC90ZXN0LnNjc3MiCgldLAoJIm5hbWVzIjogW10sCgkibWFwcGluZ3MiOiAiQUFBQSxPQUFPLENBQUMsMEJBQUk7QUFFWixBQUFBLEdBQUcsQ0FBQztFQUNGLElBQUksRUFBRSw0QkFBaUQsR0FDeEQ7O0FBSUQsQUFBQSxHQUFHLENBQUM7RUFDQSxJQUFJLEVBQUUsWUFBYSxHQUN0QiIKfQ== */
