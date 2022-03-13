#include <picogl/framework/renderers.h>
#include <picogl/framework/asset_io.h>

#define PICOGL_IMPLEMENTATION
#include <picogl/picogl.hpp>

#include <spdlog/spdlog.h>
#include <glm/gtc/type_ptr.hpp>

namespace framework
{
	void SingleColorRenderer::render(const Camera& camera, const picogl::Mesh& mesh, const glm::mat4& model, const glm::vec4& color)
	{
		m_program.use();
		const glm::mat3 model_transform = glm::transpose(glm::inverse(glm::mat3(model)));
		m_program.set_uniform("view_proj", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(camera.m_view_proj));
		m_program.set_uniform("model", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(model));
		m_program.set_uniform("normal_transform", glUniformMatrix3fv, 1, GL_FALSE, glm::value_ptr(model_transform));
		m_program.set_uniform("uniform_color", glUniform4fv, 1, glm::value_ptr(color));
		mesh.draw();
	}

	void PhongRenderer::render(const Camera& camera, const picogl::Mesh& mesh, const glm::mat4& model, const glm::vec3 light_position)
	{
		m_program.use();
		const glm::mat3 model_transform = glm::transpose(glm::inverse(glm::mat3(model)));
		m_program.set_uniform("view_proj", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(camera.m_view_proj));
		m_program.set_uniform("model", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(model));
		m_program.set_uniform("normal_transform", glUniformMatrix3fv, 1, GL_FALSE, glm::value_ptr(model_transform));
		m_program.set_uniform("light_pos", glUniform3fv, 1, glm::value_ptr(light_position));
		m_program.set_uniform("camera_pos", glUniform3fv, 1, glm::value_ptr(camera.m_position));
		mesh.draw();
	}

	void TextureRenderer::render(const picogl::Texture& tex, const glm::mat3& uv_transform, float lod)
	{
		m_program.use();
		m_program.set_uniform("screen_to_uv", glUniformMatrix3fv, 1, GL_FALSE, glm::value_ptr(uv_transform));
		m_program.set_uniform("lod", glUniform1f, lod);
		tex.bind_as_sampler(GL_TEXTURE0);
		m_dummy.draw(GL_TRIANGLES, 3);
	}

	void MultiRenderer::render(const Camera& camera, const picogl::Mesh& m, const picogl::Buffer& instance_ssbo, const picogl::Buffer& instance_offset_ssbo)
	{
		m_program.use();
		instance_ssbo.bind_as_ssbo(0);
		instance_offset_ssbo.bind_as_ssbo(1);
		m_program.set_uniform("view_proj", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(camera.m_view_proj));
		m_program.set_uniform("light_pos", glUniform3fv, 1, glm::value_ptr(camera.m_position));
		m_program.set_uniform("camera_pos", glUniform3fv, 1, glm::value_ptr(camera.m_position));
		m.draw();
	}

	void GridRenderer::render(const Camera& camera)
	{
		const glm::mat4 identity = glm::mat4(1);
		m_program.use();
		m_program.set_uniform("view_proj", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(camera.m_view_proj));
		m_program.set_uniform("model", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(identity));
		m_plane.draw();
	}

	void CubeMapRenderer::render(const Camera& camera, const picogl::Texture& cubemap)
	{
		m_program.use();
		cubemap.bind_as_sampler(GL_TEXTURE0);
		m_program.set_uniform("camera_ray_derivatives", glUniformMatrix3fv, 1, GL_FALSE, glm::value_ptr(camera.m_ray_derivatives));
		m_dummy.draw(GL_TRIANGLES, 3);
	}

	RendererCollection RendererCollection::make(const std::filesystem::path& shader_folder)
	{
		RendererCollection collection;

		auto mesh_interface_vert = picogl::Shader::make(GL_VERTEX_SHADER, make_string_from_file(shader_folder / "mesh_interface.vert"));
		auto screen_quad_vert = picogl::Shader::make(GL_VERTEX_SHADER, make_string_from_file(shader_folder / "screen_quad.vert"));
		auto multi_interface = picogl::Shader::make(GL_VERTEX_SHADER, make_string_from_file(shader_folder / "mesh_multi_draw.vert"));

		auto single_color_frag = picogl::Shader::make(GL_FRAGMENT_SHADER, make_string_from_file(shader_folder / "single_color.frag"));
		auto phong_frag = picogl::Shader::make(GL_FRAGMENT_SHADER, make_string_from_file(shader_folder / "phong.frag"));
		auto texture_frag = picogl::Shader::make(GL_FRAGMENT_SHADER, make_string_from_file(shader_folder / "texture.frag"));
		auto uber_frag = picogl::Shader::make(GL_FRAGMENT_SHADER, make_string_from_file(shader_folder / "uber_shading_multi.frag"));
		auto grid_frag = picogl::Shader::make(GL_FRAGMENT_SHADER, make_string_from_file(shader_folder / "grid.frag"));
		auto cubemap_frag = picogl::Shader::make(GL_FRAGMENT_SHADER, make_string_from_file(shader_folder / "cube_map.frag"));

		collection.m_single_color.m_program = picogl::Program::make({ mesh_interface_vert, single_color_frag });
		collection.m_phong.m_program = picogl::Program::make({ mesh_interface_vert, phong_frag });
		collection.m_texture.m_program = picogl::Program::make({ screen_quad_vert, texture_frag });

		collection.m_multi_renderer.m_program = picogl::Program::make({ multi_interface, uber_frag });
		collection.m_grid_renderer.m_program = picogl::Program::make({ mesh_interface_vert, grid_frag });

		collection.m_texture.m_dummy = picogl::Mesh::make();
		const float infty = 1e2f;
		auto plane = picogl::Mesh::make();
		const std::vector<glm::vec3> vs = { {-infty, -1.0f, -infty}, {-infty, -1.0f, +infty}, {+infty, -1.0f, -infty}, {+infty, -1.0f, +infty} };
		plane.set_indices(GL_TRIANGLE_STRIP, std::vector<GLuint>{0, 1, 2, 3});
		plane.set_vertex_attributes({
			{ vs, GL_FLOAT, 3 },
			{ std::vector<glm::vec3>(4, glm::vec3(0,0,1)), GL_FLOAT, 3 },
			{ std::vector<glm::vec2>(4, glm::vec2(0)), GL_FLOAT, 2 },
			{ std::vector<glm::vec3>(4, glm::vec3(1)), GL_FLOAT, 3 }
			});
		collection.m_grid_renderer.m_plane = std::move(plane);

		collection.m_cubemap_renderer.m_program = picogl::Program::make({ screen_quad_vert , cubemap_frag });
		collection.m_cubemap_renderer.m_dummy = picogl::Mesh::make();

		return collection;
	}
}