echo "Converting into SPIR-V headers"

mkdir -p spirv

for path in ./shaders/*; do
	[[ -f "$path" ]] || continue
	name=$(basename "$path")
	glslc $path -o - | xxd -n ${name}_contents -i > spirv/$name.h
	echo "Done $name"
done
