var a = {1, 2, 3, 4, 5};
var b = {5, 4, 1};
var c = {1, 4, 5};

assert(a.isSuper(b));
assert(a.isSuper(c));
assert(c.isSuper(b));
