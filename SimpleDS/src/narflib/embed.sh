#!/usr/bin/env bash
# generate C++ source file for embedding binary data in executable

in="$1"
out="$2"
zlib="$3"

if [ "x$1" = "x" -o "x$2" = "x" ]; then
	echo "usage: $0 <in.data> <out.cpp>"
	exit 1
fi

function size() {
	wc -c "$1" | awk '{ print $1 }'
}

# TODO: this probably doesn't cover every case,
# but it's good enough for the filenames in use right now
varbase=$in
varbase="${varbase//./_}"
varbase="${varbase//-/_}"

# CMake is dumb and regenerates the same file for multiple targets,
# so do all our work in a temporary file and then move the temporary file
# to the actual output target at the end.
# This avoids to avoid two copies of this script
# appending to the same file.
tmp=$(mktemp /tmp/tmp.XXXXXXXXXX)
rawsize=$(size "$in")

usegz=0
gzsize="n/a"
if [ "x$zlib" = "xTRUE" ]; then
	ingz=$(mktemp /tmp/tmp.XXXXXXXXXX.gz)
	gzip -9 -n -c "$in" > "$ingz"
	gzsize=$(size "$ingz")
	if [ $gzsize -lt $rawsize ]; then
		usegz=1;
	fi
fi

[ $usegz -eq 0 ] && echo -n "* " || echo -n "  "
echo "raw: $rawsize"
[ $usegz -eq 1 ] && echo -n "* " || echo -n "  "
echo "gz : $gzsize"

if [ $usegz -eq 1 ]; then
cat > "$tmp" << EOF
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
namespace narf { namespace embed {
bool uncompress(void* dst, size_t dstSize, const void* src, size_t srcSize);

static const uint8_t ${varbase}_gz[] = {
EOF

xxd -i < "$ingz" >> "$tmp"

cat >> "$tmp" << EOF
};

extern uint8_t ${varbase}_data[];
uint8_t ${varbase}_data[$rawsize];

extern const size_t ${varbase}_size;
const size_t ${varbase}_size = $rawsize;

class ${varbase}_ctor {
public:
	${varbase}_ctor() {
		bool rc = uncompress(
			${varbase}_data, sizeof(${varbase}_data),
			${varbase}_gz, sizeof(${varbase}_gz));
		assert(rc == true);
	}
};

// instance of ctor to decompress data on startup
extern ${varbase}_ctor ${varbase}_inst;
${varbase}_ctor ${varbase}_inst;
}}
EOF

else # ZLIB not found, don't compress

cat > "$tmp" << EOF
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
namespace narf { namespace embed {

extern const uint8_t ${varbase}_data[];
const uint8_t ${varbase}_data[] = {
EOF

xxd -i < "$in" >> "$tmp"

cat >> "$tmp" << EOF
};

extern const size_t ${varbase}_size;
const size_t ${varbase}_size = $rawsize;

}}
EOF

fi

mv "$tmp" "$out"
rm -f "$ingz"
