# gd-url-parser URL Parsing Library
Based on [libyuarel](https://github.com/jacketizer/libyuarel)

## Example
```gdscript
var parser = preload("res://bin/gd-url-parser.gdns").new()
parser.parse("http://thatsme:coolpassword@localhost:8989/path/to/test?query=yes&test=no#frag=1", "&")
print(parser.scheme)
print(parser.username)
print(parser.password)
print(parser.host)
print(parser.port)
print(parser.path)
print(parser.query)
print(parser.fragment)
```
Will produce:
```
http
thatsme
coolpassword
localhost
8989
path/to/test
[[query, yes], [test, no]]
frag=1
```

## TODO
- Sane building, probably gotta get rid of SCons to something like CMake to get platform coverage for free
- Check for memory leakage, as it's written in C GDNative API it's kinda manual with a lot of room to get fucked
