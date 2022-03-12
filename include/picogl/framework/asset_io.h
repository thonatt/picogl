#pragma once

#include <picogl/framework/image.h>

#include <glad/glad.h>
#include <picogl/picogl.hpp>

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <filesystem>
#include <vector>
#include <random>

namespace framework
{
	struct AABB
	{
		static AABB make_empty();

		glm::vec3 diagonal() const;
		glm::vec3 center() const;
		AABB transform(const glm::mat4x3& transfo) const;

		void extend(const glm::vec3& v);

		glm::vec3 m_min;
		glm::vec3 m_max;
	};

	struct Mesh
	{
		picogl::Mesh m_mesh;
		framework::AABB m_aabb;
	};

	AABB make_aabb(const std::vector<glm::vec3>& positions);
	std::string make_string_from_file(const std::filesystem::path& filepath);
	std::vector<Mesh> make_mesh_from_obj(const std::filesystem::path& filepath);
	picogl::Texture make_texture_from_file(const std::filesystem::path& filepath);
	Image make_image_from_file(const std::filesystem::path& filepath);

	Mesh make_cube();
	Mesh make_torus(const float R, const float r, std::uint32_t precision = 32u);
	Mesh make_sphere(std::uint32_t precision = 32u);
	Mesh make_aabb_lines(const AABB& aabb);

	Image make_perlin(std::uint32_t w, std::uint32_t h, std::uint32_t size);
	Image make_checkers(std::uint32_t w, std::uint32_t h, std::uint32_t size);

	picogl::Texture make_texture_from_image(const Image& src, GLenum internal_format);
	picogl::Texture make_cubemap_from_file(const std::filesystem::path& filepath, GLenum internal_format);

	namespace impl
	{
		float smoothstep(const float a, const float b, const float t);

		template<typename T, int N>
		glm::vec<N, T> make_random_vec();

		template<typename T, int N>
		glm::vec<N, T> make_random_direction();

		template<typename T>
		picogl::Mesh make_triangle_mesh(
			const std::vector<T>& tris,
			const std::vector<glm::vec3>& ps,
			const std::vector<glm::vec3>& ns,
			const std::vector<glm::vec2>& uv,
			const std::vector<glm::vec3>& cs);

		template<typename T, int N>
		glm::vec<N, T> make_random_vec()
		{
			static std::random_device device;
			static std::mt19937 generator(device());

			std::uniform_real_distribution<T> distribution(T(-1), T(1));

			glm::vec<N, T> out;
			for (int i = 0; i < N; ++i)
				out[i] = distribution(generator);

			return out;
		}

		template<typename T, int N>
		glm::vec<N, T> make_random_direction()
		{
			glm::vec<N, T> out;
			do
				out = make_random_vec<T, N>();
			while (glm::dot(out, out) > T(1));
			return glm::normalize(out);
		}

		template<typename T>
		picogl::Mesh make_triangle_mesh(
			const std::vector<T>& tris,
			const std::vector<glm::vec3>& ps,
			const std::vector<glm::vec3>& ns,
			const std::vector<glm::vec2>& uv,
			const std::vector<glm::vec3>& cs)
		{
			picogl::Mesh mesh = picogl::Mesh::make();
			mesh.set_indices(GL_TRIANGLES, tris, GL_UNSIGNED_INT);
			mesh.set_vertex_attributes({
				{ps, GL_FLOAT, 3},
				{ns, GL_FLOAT, 3},
				{uv, GL_FLOAT, 2},
				{cs, GL_FLOAT, 3}
				});

			return mesh;
		}
	}
}
