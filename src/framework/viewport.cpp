#include <picogl/framework/viewport.h>

#define PICOGL_IMPLEMENTATION
#include <picogl/picogl.hpp>

#include <glm/gtx/transform.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace framework
{
	constexpr float pi = 3.14159265f;

	Viewport::Viewport(const std::string& name, const std::vector<GLenum>& additional_attachments)
		: m_name{ name }, m_additional_attachments{ additional_attachments }
	{
	}

	void Viewport::gui()
	{
		if (ImGui::Begin(m_name.c_str(), nullptr, ImGuiWindowFlags_NoNav))
		{
			if (m_framebuffer.sample_count() > 1) {
				m_framebuffer.blit_to(m_resolve_framebuffer);
				for (GLsizei i = 0; i < m_additional_attachments.size(); ++i) {
					const GLenum attachment = GL_COLOR_ATTACHMENT1 + i;
					m_framebuffer.blit_to(m_resolve_framebuffer, attachment, GL_NEAREST, attachment);
				}
			}

			const picogl::Texture& color_attachment = final_framebuffer().color_attachments().front();
			const ImVec2& content = ImGui::GetContentRegionAvail();
			ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<std::size_t>(color_attachment)), content);
			m_vp_size = glm::ivec2(ImGui::GetItemRectSize().x, ImGui::GetItemRectSize().y);
			m_vp_position = glm::ivec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y);

			gui_body();
		}
		ImGui::End();
	}

	void Viewport::resize(GLsizei width, GLsizei height, GLsizei sample_count)
	{
		width = glm::max(width, 1);
		height = glm::max(height, 1);
		sample_count = glm::max(sample_count, 1);
		m_sample_count = sample_count;

		auto setup_framebuffer = [](picogl::Framebuffer& framebuffer, const std::vector<GLenum>& additional_attachments)
		{
			framebuffer.set_depth_attachment(GL_DEPTH_COMPONENT32);
			framebuffer.add_color_attachment(GL_RGBA8);
			for (const GLenum format : additional_attachments)
				framebuffer.add_color_attachment(format);
		};

		if (m_framebuffer.width() != width || m_framebuffer.height() != height || m_framebuffer.sample_count() != sample_count) {
			m_framebuffer = picogl::Framebuffer::make(width, height, sample_count);
			setup_framebuffer(m_framebuffer, m_additional_attachments);

			if (sample_count > 1)
			{
				m_resolve_framebuffer = picogl::Framebuffer::make(width, height, 1);
				setup_framebuffer(m_resolve_framebuffer, m_additional_attachments);
			}
		}
	}

	void Viewport::update()
	{
		resize(GLsizei(m_vp_size.x), GLsizei(m_vp_size.y), m_sample_count);
	}

	const picogl::Framebuffer& Viewport::final_framebuffer() const
	{
		return m_framebuffer.sample_count() > 1 ? m_resolve_framebuffer : m_framebuffer;
	}

	Viewport2D::Viewport2D(const::std::string& name, const::std::vector<GLenum>& additional_attachments)
		: Viewport(name, additional_attachments)
	{
	}

	void Viewport2D::gui_body()
	{
		const ImGuiIO& io = ImGui::GetIO();

		if (!ImGui::IsWindowFocused()) {
			m_clicked_position.x = -FLT_MAX;
			return;
		}

		const glm::vec2 mouse_pos_screen = (glm::vec2(io.MousePos.x, io.MousePos.y) - m_vp_position) / m_vp_size;
		if (ImGui::IsItemHovered()
			&& 0 < mouse_pos_screen.x && mouse_pos_screen.x < 1
			&& 0 < mouse_pos_screen.y && mouse_pos_screen.y < 1)
		{
			if (io.MouseWheel != 0.0f)
				m_scaling *= glm::pow(1.1f, -io.MouseWheel);

			if (ImGui::IsMouseClicked(GLFW_MOUSE_BUTTON_LEFT, false) || ImGui::IsMouseClicked(GLFW_MOUSE_BUTTON_RIGHT, false)) {
				m_clicked_position = mouse_pos_screen;
				m_clicked_translation = m_translation;
				m_clicked_angle = m_angle;
			}

			if (m_clicked_position.x != -FLT_MAX)
			{
				if (ImGui::IsMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
					const glm::vec2 src_dir = m_clicked_position - glm::vec2(0.5f);
					const glm::vec2 dst_dir = mouse_pos_screen - glm::vec2(0.5f);
					m_angle = m_clicked_angle - std::atan2(glm::determinant(glm::mat2(src_dir, dst_dir)), glm::dot(src_dir, dst_dir));
				}

				if (ImGui::IsMouseDown(GLFW_MOUSE_BUTTON_LEFT))
					m_translation = m_clicked_translation + glm::mat2(m_screen_to_uv) * (m_clicked_position - mouse_pos_screen);

				if (ImGui::IsMouseReleased(GLFW_MOUSE_BUTTON_LEFT) || ImGui::IsMouseReleased(GLFW_MOUSE_BUTTON_RIGHT))
					m_clicked_position.x = -FLT_MAX;
			}
		}
	}

	void Viewport2D::update()
	{
		Viewport::update();

		constexpr glm::mat3 recenter = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(-0.5f, -0.5f, 1));
		const glm::mat3 translate = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(m_translation, 1));
		const glm::mat3 scale = glm::mat3(glm::vec3(m_scaling.x, 0, 0), glm::vec3(0, m_scaling.y, 0), glm::vec3(0, 0, 1));
		const float cosa = std::cos(m_angle), sina = std::sin(m_angle);
		const glm::mat3 rotate = glm::mat3(glm::vec3(cosa, sina, 0), glm::vec3(-sina, cosa, 0), glm::vec3(0, 0, 1));
		m_screen_to_uv = translate * rotate * scale * recenter;
	}

	Viewport3D::Viewport3D(const::std::string& name, const::std::vector<GLenum>& additional_attachments)
		: Viewport(name, additional_attachments)
	{
	}

	void Viewport3D::resize(GLsizei width, GLsizei height, GLsizei sample_count)
	{
		Viewport::resize(width, height, sample_count);

		m_camera.m_w = float(width);
		m_camera.m_h = float(height);
		m_camera.update();
	}

	void Viewport3D::gui_body()
	{
		const ImGuiIO& io = ImGui::GetIO();

		if (!ImGui::IsWindowFocused()) {
			m_clicked_position.x = -FLT_MAX;
			return;
		}

		if (ImGui::IsItemHovered())
		{
			if (io.MouseWheel != 0.0f) {
				const float ratio = std::pow(1.05f, -io.MouseWheel);
				if (ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
					if (ImGui::IsKeyDown(GLFW_KEY_LEFT_SHIFT))
						m_camera.m_near = std::min(m_camera.m_near / ratio, m_camera.m_far);
					else
						m_camera.m_far = std::max(m_camera.m_far / ratio, m_camera.m_near);
				} else
					m_camera.m_position = m_camera.m_target + ratio * (m_camera.m_position - m_camera.m_target);
			}

			if (ImGui::IsKeyDown(GLFW_KEY_E) || ImGui::IsKeyDown(GLFW_KEY_D)){
				glm::vec3 delta = (0.1f / 60.0f) * (m_camera.m_target - m_camera.m_position);
				if (ImGui::IsKeyDown(GLFW_KEY_D))
					delta = -delta;
				m_camera.m_position += delta;
				m_camera.m_target += delta;
			}

			if (io.MousePos.x != -FLT_MAX && io.MousePos.y != FLT_MAX) {
				if (io.MouseDownDuration[GLFW_MOUSE_BUTTON_RIGHT] == 0.0 || io.MouseDownDuration[GLFW_MOUSE_BUTTON_LEFT] == 0.0 ||
					io.MouseDownDuration[GLFW_MOUSE_BUTTON_MIDDLE] == 0.0)
				{
					m_clicked_position = { io.MousePos.x, io.MousePos.y };
					m_clicked_camera = m_camera;
				}

				if (m_clicked_position.x != -FLT_MAX)
				{
					const glm::vec2 current_pos = { io.MousePos.x, io.MousePos.y };
					const glm::vec2 delta_screen = (current_pos - m_clicked_position) / glm::vec2(m_camera.m_w, m_camera.m_h);
					const glm::vec2 mouse_pos_screen = (current_pos - m_vp_position) / m_vp_size;
					const glm::vec3 direction = m_clicked_camera.m_position - m_clicked_camera.m_target;
					if (io.MouseDownDuration[GLFW_MOUSE_BUTTON_RIGHT] > 0.0f) {
						const float scaling = glm::length(m_diagonal);
						const glm::vec3 delta_world = scaling * glm::mat3(m_clicked_camera.m_inverse_view) * glm::vec3(delta_screen, 0.0f);
						m_camera.m_position = m_clicked_camera.m_position - delta_world;
						m_camera.m_target = m_clicked_camera.m_target - delta_world;
					} else if (io.MouseDownDuration[GLFW_MOUSE_BUTTON_LEFT] > 0.0f) {
						const glm::vec2 angles = delta_screen * glm::vec2(-pi, pi / 2.0f);
						const glm::mat3 rot_x = glm::mat3(glm::rotate(glm::mat4(1.0f), angles[0], m_clicked_camera.up()));
						const glm::mat3 rot_y = glm::mat3(glm::rotate(glm::mat4(1.0f), angles[1], m_clicked_camera.right()));
						const glm::vec3 delta_world = rot_y * (rot_x * direction);
						m_camera.m_position = m_clicked_camera.m_target + delta_world;
						m_camera.m_up = glm::vec3(rot_y * (rot_x * glm::vec4(m_clicked_camera.m_up, 0.0)));
					} else if (io.MouseDownDuration[GLFW_MOUSE_BUTTON_MIDDLE] > 0.0f) {
						const glm::vec2 src_dir = m_clicked_position - glm::vec2(0.5f);
						const glm::vec2 dst_dir = mouse_pos_screen - glm::vec2(0.5f);
						const float angle = std::atan2(glm::determinant(glm::mat2(src_dir, dst_dir)), glm::dot(src_dir, dst_dir));
						const glm::mat3 rot_z = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, m_clicked_camera.front()));
						const glm::vec3 delta_world = rot_z * direction;
						m_camera.m_position = m_clicked_camera.m_target + delta_world;
						m_camera.m_up = rot_z * m_clicked_camera.m_up;
					}
				}
			}
		}
	}
}