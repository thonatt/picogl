#version 460

layout(location = 0) out vec4 out_color;
layout(binding = 0) uniform sampler3D density; 

in VertexData {
	vec3 position, normal, color;
	vec2 uv;
} frag_in;

uniform vec3 eye_pos;
uniform vec3 box_min = 0.5*vec3(-1);
uniform vec3 box_max = 0.5*vec3(+1);
uniform ivec3 grid_size;
uniform float intensity = 1.0f;

float sample_density(ivec3 cell) {
	return texture(density, (vec3(cell) + 0.5)/vec3(grid_size)).x;
}

float max_coef(vec3 v){
	return max(v.x, max(v.y, v.z));
}

float min_coef(vec3 v){
	return min(v.x, min(v.y, v.z));
}

ivec3 get_cell(vec3 p){
	vec3 uv = (p - box_min)/(box_max - box_min);
	return ivec3(floor(vec3(grid_size) * uv));
}

int get_min_index(vec3 v) {
	return v.x <= v.y ? (v.x <= v.z ? 0 : 2 ) : (v.y <= v.z ? 1 : 2);
}

bool inside_box(vec3 p){
	return all(greaterThanEqual(p, box_min)) && all(greaterThanEqual(box_max, p));
}

float box_intersection(vec3 ray_dir) {
	vec3 min_ts = (box_min - eye_pos)/ray_dir;
	vec3 max_ts = (box_max - eye_pos)/ray_dir;

	float near_t = max_coef(min(min_ts, max_ts)); 
	float far_t = min_coef(max(min_ts, max_ts)); 
	if( 0 <= near_t && near_t <= near_t){
		return near_t;
	}
	return -1.0;
}

void main(){

	vec3 dir = normalize(frag_in.position - eye_pos);

	vec3 start = eye_pos;
			
	if(!inside_box(eye_pos)){
		float box_dist = box_intersection(dir);
		if(box_dist >= 0)
			start = eye_pos + box_dist*dir;
		else
			discard;
	} 

	vec3 cell_size = (box_max - box_min)/vec3(grid_size); 
	start = clamp(start, box_min, box_max - 0.001*cell_size);
	ivec3 current_cell = get_cell(start);

	vec3 deltas = cell_size / abs(dir);
	vec3 fracs = fract((start - box_min)/cell_size);			
	vec3 ts;
	ivec3 final_cell, steps;
	for(int k=0; k<3; ++k){
		steps[k] = (dir[k] >= 0 ? 1 : -1);
		ts[k] = deltas[k] * (dir[k] >= 0 ? 1.0 - fracs[k] : fracs[k]);
		final_cell[k] = (dir[k] >= 0 ? grid_size[k] : -1);
	}

	float t = 0.0f;
	float alpha = 0.0f;
	do {
		int c = get_min_index(ts);
		current_cell[c] += steps[c];

		float delta_t = ts[c] - t;
		t = ts[c];

		ts[c] += deltas[c];
		
		alpha += delta_t * sample_density(current_cell).x;
	} while (all(notEqual(current_cell, final_cell)));
	
	out_color = vec4(mix(vec3(1,1,0), vec3(1), intensity*alpha),  min(5.0*alpha, 1.0));
}