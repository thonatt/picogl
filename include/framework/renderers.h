#pragma once

#include <framework/include/camera.h>

#include <glad/glad.h>
#include <picogl.hpp>

#include <glm/glm.hpp>
#include <filesystem>

namespace framework
{
	struct Renderer
	{
		picogl::Program m_program;
	};

	struct SingleColorRenderer : Renderer
	{
		void render(const Camera& camera, const picogl::Mesh& mesh, const glm::mat4& model, const glm::vec4& color);
	};

	struct PhongRenderer : Renderer
	{
		void render(const Camera& camera, const picogl::Mesh& mesh, const glm::mat4& model, const glm::vec3 light_position);
	};

	struct TextureRenderer : Renderer
	{
		void render(const picogl::Texture& tex, const glm::mat3& uv_transform, float lod = -1.0f);
		picogl::Mesh m_dummy;
	};

	struct MultiRenderer : Renderer
	{
		void render(const Camera& camera, const picogl::Mesh& m, const picogl::Buffer& instance_ssbo, const picogl::Buffer& instance_offset_ssbo);
	};

	struct GridRenderer : Renderer
	{
		void render(const Camera& camera);
		picogl::Mesh m_plane;
	};

	struct CubeMapRenderer : Renderer
	{
		void render(const Camera& camera, const picogl::Texture& cubemap);
		picogl::Mesh m_dummy;
	};

	struct RendererCollection
	{
		static RendererCollection make(const std::filesystem::path& shader_folder);

		SingleColorRenderer m_single_color;
		PhongRenderer m_phong;
		TextureRenderer m_texture;
		MultiRenderer m_multi_renderer;
		GridRenderer m_grid_renderer;
		CubeMapRenderer m_cubemap_renderer;
	};
}
