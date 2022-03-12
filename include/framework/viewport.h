#pragma once

#include <framework/include/camera.h>

#include <glad/glad.h>
#include <picogl.hpp>

#include <glm/glm.hpp>

#include <vector>
#include <string>

namespace framework
{
	class Viewport
	{
	public:

		Viewport(const std::string& name, const std::vector<GLenum>& additional_attachments = {});

		void gui();

		virtual void resize(GLsizei width, GLsizei height, GLsizei sample_count = 1);
		virtual void gui_body() = 0;
		virtual void update();

		const picogl::Framebuffer& final_framebuffer() const;

	protected:
		picogl::Framebuffer m_framebuffer;
		picogl::Framebuffer m_resolve_framebuffer;
		GLsizei m_sample_count = 1;
		glm::vec2 m_clicked_position = {};
		glm::vec2 m_vp_size = glm::vec2(1);
		glm::vec2 m_vp_position = {};
		std::string m_name = {};
		std::vector<GLenum> m_additional_attachments;
	};

	class Viewport2D : public Viewport
	{
	public:
		Viewport2D(const std::string& name, const std::vector<GLenum>& additional_attachments = {});

		virtual void gui_body() override;
		virtual void update() override;

		glm::mat3 m_screen_to_uv = glm::mat3(1.0f);
		glm::vec2 m_translation = glm::vec2(0.5f);
		glm::vec2 m_scaling = glm::vec2(1.5f);
		float m_angle = 0.0f;

		glm::vec2 m_clicked_translation = {};
		float m_clicked_angle = {};
	};

	class Viewport3D : public Viewport
	{
	public:
		Viewport3D(const std::string& name, const std::vector<GLenum>& additional_attachments = {});

		void resize(GLsizei width, GLsizei height, GLsizei sample_count = 1) override;
		virtual void gui_body() override;

		glm::vec3 m_diagonal = glm::vec3(1);
		Camera m_camera = {};
		Camera m_clicked_camera = {};
	};
}
