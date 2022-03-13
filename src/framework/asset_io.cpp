#include <picogl/framework/asset_io.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glad/glad.h>
#define PICOGL_IMPLEMENTATION
#include <picogl/picogl.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include <string>
#include <queue>

namespace framework
{
	constexpr float pi = 3.14159265f;

	AABB make_aabb(const std::vector<glm::vec3>& positions)
	{
		AABB aabb = AABB::make_empty();
		for (const glm::vec3& p : positions)
			aabb.extend(p);

		return aabb;
	}

	std::string make_string_from_file(const std::filesystem::path& filepath)
	{
		std::string str;
		std::ifstream stream(filepath, std::ios::in | std::ios::binary);
		if (stream)
		{
			stream.seekg(0, std::ios::end);
			str.resize(stream.tellg());
			stream.seekg(0, std::ios::beg);
			stream.read(str.data(), str.size());
			stream.close();
		} else
			spdlog::error("Can't read {}", std::filesystem::absolute(filepath).string());

		return str;
	}

	std::vector<Mesh> make_mesh_from_obj(const std::filesystem::path& filepath)
	{
		struct Vertex {
			glm::vec3 m_position;
			glm::vec3 m_normal;
			glm::vec3 m_color;
			glm::vec2 m_uv;

			bool operator==(const Vertex& v) const {
				return m_position == v.m_position && m_uv == v.m_uv;
			}

			struct Hash {
				std::size_t operator()(const Vertex& v) const {
					const std::size_t h1 = std::hash<glm::vec3>{}(v.m_position);
					return h1 ^ (std::hash<glm::vec2>{}(v.m_uv) + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
				}
			};
		};

		tinyobj::ObjReaderConfig reader_config;
		reader_config.mtl_search_path = "./"; // Path to material files

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(filepath.string(), reader_config)) {
			if (!reader.Error().empty())
				spdlog::error("TinyObjReader: {}", reader.Error());
			return {};
		}

		if (!reader.Warning().empty())
			spdlog::warn("TinyObjReader: {}", reader.Warning());

		const auto& attrib = reader.GetAttrib();
		const auto& shapes = reader.GetShapes();
		const auto& materials = reader.GetMaterials();

		std::vector<Mesh> meshes;
		for (const tinyobj::shape_t& shape : shapes)
		{
			Mesh mesh;
			std::unordered_map<Vertex, std::uint32_t, Vertex::Hash> unique_vertices;
			std::vector<std::uint32_t> indices;

			std::size_t index_offset = 0;
			for (std::size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
			{
				const std::size_t fv = std::size_t(shape.mesh.num_face_vertices[f]);
				for (std::size_t v = 0; v < fv; v++)
				{
					const tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

					Vertex vertex;
					vertex.m_position = {
						attrib.vertices[3 * std::size_t(idx.vertex_index) + 0],
						attrib.vertices[3 * std::size_t(idx.vertex_index) + 1],
						attrib.vertices[3 * std::size_t(idx.vertex_index) + 2]
					};

					if (idx.texcoord_index >= 0) {
						vertex.m_uv = {
							attrib.texcoords[2 * std::size_t(idx.texcoord_index) + 0],
							attrib.texcoords[2 * std::size_t(idx.texcoord_index) + 1]
						};
					} else
						vertex.m_uv = glm::vec2(0.5);

					const auto it = unique_vertices.find(vertex);
					if (it != unique_vertices.end())
						indices.push_back(it->second);
					else
					{
						if (idx.normal_index >= 0) {
							vertex.m_normal = {
								attrib.normals[3 * std::size_t(idx.normal_index) + 0],
								attrib.normals[3 * std::size_t(idx.normal_index) + 1],
								attrib.normals[3 * std::size_t(idx.normal_index) + 2]
							};
						} else
							vertex.m_normal = glm::vec3(0, 0, 1);

						vertex.m_color = {
							attrib.colors[3 * std::size_t(idx.vertex_index) + 0],
							attrib.colors[3 * std::size_t(idx.vertex_index) + 1],
							attrib.colors[3 * std::size_t(idx.vertex_index) + 2]
						};

						const std::uint32_t index = std::uint32_t(unique_vertices.size());
						unique_vertices.emplace_hint(it, std::move(vertex), index);
						indices.push_back(index);
					}
				}
				index_offset += fv;
			}

			const std::size_t v_count = unique_vertices.size();
			std::vector<glm::vec3> pos(v_count), col(v_count), ns(v_count);
			std::vector<glm::vec2> uvs(v_count);
			for (const auto& it : unique_vertices) {
				const Vertex& v = it.first;
				const std::uint32_t i = it.second;
				pos[i] = v.m_position;
				ns[i] = v.m_normal;
				col[i] = v.m_color;
				uvs[i] = v.m_uv;
			}

			mesh.m_aabb = make_aabb(pos);
			mesh.m_mesh = utils::make_triangle_mesh(indices, pos, ns, uvs, col);
			meshes.push_back(std::move(mesh));
		}

		return meshes;
	}

	picogl::Texture make_texture_from_file(const std::filesystem::path& filepath)
	{
		const Image img = make_image_from_file(filepath);
		switch (img.m_channel_count)
		{
		case 4:
			return make_texture_from_image(img, GL_RGBA8);
		case 3:
			return make_texture_from_image(img, GL_RGB8);
		case 1:
			return make_texture_from_image(img, GL_R8);
		default:
			return {};
		}
	}

	Image make_image_from_file(const std::filesystem::path& filepath)
	{
		int w, h, c;
		stbi_uc* data = stbi_load(filepath.string().c_str(), &w, &h, &c, 0);
		if (!data) {
			spdlog::warn("Cant load {}", filepath.string());
			return {};
		}

		Image dst;
		switch (c)
		{
		case 4:
			dst = Image::make<glm::u8vec4>(w, h, 4, data);
			break;
		case 3:
		{
			dst = Image::make<glm::u8vec4>(w, h, 4);
			stbi_uc* src_ptr = data;
			for (std::uint32_t y = 0; y < dst.m_height; ++y)
				for (std::uint32_t x = 0; x < dst.m_width; ++x, src_ptr += 3)
					dst.at<glm::u8vec4>(x, y) = glm::u8vec4(*src_ptr, *(src_ptr + 1), *(src_ptr + 2), 255);
			break;
		}
		case 2:
		{
			dst = Image::make<glm::u8vec2>(w, h, 2, data);
			break;
		}
		case 1:
			dst = Image::make<std::byte>(w, h, 1, data);
			break;
		default:
			break;
		}

		stbi_image_free(data);

		return dst;
	}

	Mesh make_cube()
	{
		static const std::vector<glm::uvec3> tris = {
			{ 0, 3, 1 }, { 0, 2, 3 },
			{ 4, 5, 7 }, {7, 6, 4 },
			{ 8, 11, 9 }, { 11, 8, 10 },
			{ 12, 13, 15 }, { 12, 15, 14 },
			{ 16, 19, 17 }, { 19, 16, 18 },
			{ 20, 21, 23 }, { 20, 23, 22 }
		};

		constexpr std::array<glm::ivec4, 6> faces = {
			//top bottom left right floor ceil
			glm::ivec4{ 2, 3, 6, 7 },
			glm::ivec4{ 0, 1, 4, 5 },
			glm::ivec4{ 0, 2, 4, 6 },
			glm::ivec4{ 1, 3, 5, 7 },
			glm::ivec4{ 0, 1, 2, 3 },
			glm::ivec4{ 4, 5, 6, 7 }
		};

		constexpr std::array<glm::vec3, 8> corners_positions = {
			glm::vec3{-1, -1, -1},
			glm::vec3{+1, -1, -1},
			glm::vec3{-1, +1, -1},
			glm::vec3{+1, +1, -1},
			glm::vec3{-1, -1, +1},
			glm::vec3{+1, -1, +1},
			glm::vec3{-1, +1, +1},
			glm::vec3{+1, +1, +1},
		};

		std::vector<glm::vec3> positions(4 * 6);
		std::vector<glm::vec2> uvs(4 * 6);
		for (int f = 0; f < 6; ++f) {
			for (int v = 0; v < 4; ++v) {
				positions[4 * f + v] = corners_positions[faces[f][v]];
				uvs[4 * f + v] = glm::vec2(v / 2, v % 2);
			}
		}

		Mesh mesh;
		mesh.m_aabb = make_aabb(positions);
		mesh.m_mesh = utils::make_triangle_mesh(tris, positions, std::vector<glm::vec3>(positions.size(), glm::vec3(1)), uvs, std::vector<glm::vec3>(positions.size(), glm::vec3(1)));
		return mesh;
	}

	Mesh make_torus(const float R, const float r, std::uint32_t precision)
	{
		precision = std::max(precision, 2u);
		std::vector<glm::vec3> positions((precision + 1u) * precision);
		std::vector<glm::vec3> normals(positions.size());
		std::vector<glm::vec2> uvs(positions.size());
		std::vector<glm::uvec3> triangles(2u * precision * (precision - 1u));

		const float frac = 1.0f / ((float)precision - 1.0f);
		const float frac_uv = 1.0f / (float)precision;

		std::uint32_t v_id = 0;
		for (std::uint32_t t = 0; t < precision; ++t) {
			const float theta = (2.0f * t * frac + 1.0f) * pi;
			const float cost = std::cos(theta), sint = std::sin(theta);
			const glm::vec2 pos = glm::vec2(R + r * cost, r * sint);

			for (std::uint32_t p = 0; p < precision + 1; ++p, ++v_id) {
				const float phi = p * frac_uv * (2.0f * pi);
				const float cosp = std::cos(phi), sinp = std::sin(phi);
				const std::uint32_t index = p + (precision + 1) * t;
				positions[v_id] = glm::vec3(pos.x * cosp, pos.x * sinp, pos.y);
				normals[v_id] = glm::vec3(cost * cosp, cost * sinp, sint);
				uvs[v_id] = glm::vec2(t * frac_uv, p * frac_uv);
			}
		}

		std::uint32_t tri_id = 0;
		for (std::uint32_t t = 0; t < precision - 1u; ++t) {
			for (std::uint32_t p = 0; p < precision; ++p, tri_id += 2u) {
				const std::uint32_t current_id = p + (precision + 1u) * t;
				const std::uint32_t next_in_row = current_id + 1u;
				const std::uint32_t next_in_col = current_id + precision + 1u;
				const std::uint32_t next_next = next_in_col + 1u;
				triangles[tri_id] = glm::uvec3(current_id, next_in_row, next_in_col);
				triangles[tri_id + 1] = glm::uvec3(next_in_row, next_next, next_in_col);
			}
		}

		Mesh mesh;
		mesh.m_aabb = make_aabb(positions);
		mesh.m_mesh = utils::make_triangle_mesh(triangles, positions, normals, uvs, std::vector<glm::vec3>(positions.size(), glm::vec3(1)));
		return mesh;
	}

	Mesh make_sphere(std::uint32_t precision)
	{
		precision = std::max(precision, 2u);
		std::vector<glm::vec3> positions((precision + 1u) * precision);
		std::vector<glm::vec3> normals(positions.size());
		std::vector<glm::vec2> uvs(positions.size());
		std::vector<glm::uvec3> triangles(2u * precision * (precision - 1u));

		const float frac_p = 1.0f / (float)precision;
		const float frac_t = 1.0f / ((float)precision - 1.0f);

		std::uint32_t v_id = 0;
		for (std::uint32_t t = 0; t < precision; ++t) {
			const float theta = t * frac_t * pi;
			const float cost = std::cos(theta), sint = std::sin(theta);

			for (std::uint32_t p = 0; p < precision + 1; ++p, ++v_id) {
				const float phi = p * frac_p * (2.0f * pi);
				const float cosp = std::cos(phi), sinp = std::sin(phi);
				normals[v_id] = positions[v_id] = glm::vec3(sint * cosp, sint * sinp, cost);
				uvs[v_id] = glm::vec2(t * frac_t, p * frac_p);
			}
		}

		std::uint32_t tri_id = 0;
		for (std::uint32_t t = 0; t < precision - 1; ++t) {
			for (std::uint32_t p = 0; p < precision; ++p, tri_id += 2) {
				const std::uint32_t current_id = p + (precision + 1) * t;
				const std::uint32_t next_in_row = current_id + 1;
				const std::uint32_t next_in_col = current_id + precision + 1;
				const std::uint32_t next_next = next_in_col + 1;
				triangles[tri_id] = glm::uvec3(current_id, next_in_col, next_in_row);
				triangles[tri_id + 1] = glm::uvec3(next_in_row, next_in_col, next_next);
			}
		}

		Mesh mesh;
		mesh.m_aabb = { glm::vec3(-1), glm::vec3(1) };
		mesh.m_mesh = utils::make_triangle_mesh(triangles, positions, normals, uvs, std::vector<glm::vec3>(positions.size(), glm::vec3(1)));
		return mesh;
	}

	Mesh make_aabb_lines(const AABB& aabb)
	{
		static const std::vector<glm::uvec2> lines = {
			{ 0, 4 }, { 5, 1 }, { 4, 5,}, { 0, 1 },
			{ 2, 6 }, { 7, 3 }, { 6, 7,}, { 2, 3 },
			{ 0, 2 }, { 1, 3 }, { 4, 6,}, { 5, 7 }
		};

		static const std::vector<glm::vec3> unit_positions = {
			{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1},
			{1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {1, 1, 1},
		};

		const glm::vec3 center = aabb.center();
		const glm::vec3 scale = aabb.diagonal();
		const glm::mat4 transfo = glm::translate(center) * glm::scale(scale) * glm::translate(-glm::vec3(0.5f));

		std::vector<glm::vec3> positions(unit_positions.size());
		std::transform(unit_positions.begin(), unit_positions.end(), positions.begin(), [&transfo](const glm::vec3& p) {
			return glm::vec3(transfo * glm::vec4(p, 1));
			});

		picogl::Mesh mesh = picogl::Mesh::make();
		mesh.set_indices(GL_LINES, lines, GL_UNSIGNED_INT);
		mesh.set_vertex_attributes({ {positions, GL_FLOAT, 3} });

		return Mesh{ std::move(mesh) };
	}

	picogl::Texture make_texture_from_image(const Image& src, const GLenum internal_format)
	{
		return picogl::Texture::make_2d(
			internal_format,
			src.m_width,
			src.m_height,
			1,
			1,
			src.m_pixels.data(),
			picogl::Texture::Options::AutomaticAlignment | picogl::Texture::Options::GenerateMipmap);
	}

	picogl::Texture make_cubemap_from_file(const std::filesystem::path& filepath, GLenum internal_format)
	{
		const Image img = make_image_from_file(filepath);

		const int w = img.m_width / 4;
		const int h = img.m_height / 3;

		picogl::Texture dst = picogl::Texture::make_cubemap(internal_format, w, h);

		constexpr std::array<int, 6> faces = { 3, 1, 0, 5, 2, 4 };
		constexpr std::array<int, 6> x_offets = { 1, 0, 1, 2, 3, 1 };
		constexpr std::array<int, 6> y_offets = { 0, 1, 1, 1, 1, 2 };
		for (int i = 0; i < 6; ++i) {
			const Image face = img.extract_roi(w * x_offets[faces[i]], h * y_offets[faces[i]], w, h);
			dst.upload_data(face.m_pixels.data(), 0, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
		}
		return dst;
	}

	Image make_perlin(std::uint32_t w, std::uint32_t h, std::uint32_t size)
	{
		size = std::max(size, 1u);
		Image grads = Image::make<glm::vec2>(w / size + 1, h / size + 1, 2);
		for (std::uint32_t y = 0; y < grads.m_height; ++y)
			for (std::uint32_t x = 0; x < grads.m_width; ++x)
				grads.at<glm::vec2>(x, y) = utils::make_random_direction<float, 2>();

		Image dst = Image::make<glm::vec4>(w, h, 4);
		const float ratio = 1.0f / size;
		for (std::uint32_t y = 0; y < h; ++y) {
			const std::uint32_t iy = y / size;
			const float fy = y * ratio;
			const float dy = fy - (float)iy;

			for (std::uint32_t x = 0; x < w; ++x) {
				const std::uint32_t ix = x / size;
				const float fx = x * ratio;
				const float dx = fx - (float)ix;

				const float vx0 = utils::smoothstep(
					glm::dot(grads.at<glm::vec2>(ix + 0, iy + 0), glm::vec2(fx - (ix + 0), fy - (iy + 0))),
					glm::dot(grads.at<glm::vec2>(ix + 1, iy + 0), glm::vec2(fx - (ix + 1), fy - (iy + 0))),
					dx
				);

				const float vx1 = utils::smoothstep(
					glm::dot(grads.at<glm::vec2>(ix + 0, iy + 1), glm::vec2(fx - (ix + 0), fy - (iy + 1))),
					glm::dot(grads.at<glm::vec2>(ix + 1, iy + 1), glm::vec2(fx - (ix + 1), fy - (iy + 1))),
					dx
				);

				const float f = 0.5f * utils::smoothstep(vx0, vx1, dy) + 0.5f;
				dst.at<glm::vec4>(x, y) = { f, f, f , 1.0f };
			}
		}

		return dst;
	}

	Image make_checkers(std::uint32_t w, std::uint32_t h, std::uint32_t size)
	{
		size = std::max(size, 1u);
		Image dst = Image::make<glm::vec4>(w, h, 4);
		for (std::uint32_t y = 0; y < h; ++y) {
			for (std::uint32_t x = 0; x < w; ++x) {
				const float f = (y / size + x / size) % 2 ? 1.0f : 0.0f;
				dst.at<glm::vec4>(x, y) = { f, f, f, f };
			}
		}
		return dst;
	}

	AABB AABB::make_empty()
	{
		return { glm::vec3(std::numeric_limits<float>::infinity()), glm::vec3(-std::numeric_limits<float>::infinity()) };
	}

	glm::vec3 AABB::diagonal() const
	{
		return m_max - m_min;
	}

	glm::vec3 AABB::center() const
	{
		return 0.5f * (m_max + m_min);
	}

	AABB AABB::transform(const glm::mat4x3& transfo) const
	{
		const glm::vec3 tcenter = transfo * glm::vec4(center(), 1);
		const glm::mat3 abs_transfo = glm::mat3(glm::abs(transfo[0]), glm::abs(transfo[1]), glm::abs(transfo[2]));
		const glm::vec3 half_diag = 0.5f * (abs_transfo * diagonal());
		return { tcenter - half_diag, tcenter + half_diag };
	}

	void AABB::extend(const glm::vec3& v)
	{
		m_min = glm::min(m_min, v);
		m_max = glm::max(m_max, v);
	}

	namespace utils
	{
		float smoothstep(const float a, const float b, const float t)
		{
			const float u = t * t * (3.0f - 2.0f * t);
			return glm::mix(a, b, u);
		}
	}
}


