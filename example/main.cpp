#include <picogl/framework/application.h>
#include <picogl/framework/renderers.h>
#include <picogl/framework/asset_io.h>
#include <picogl/framework/viewport.h>
#include <picogl/framework/image.h>

#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#define PICOGL_IMPLEMENTATION
#include <picogl/picogl.hpp>

#include <algorithm>
#include <filesystem>
#include <vector>

static std::string shader_path;

void debug_gl()
{
	std::string err;
	if (picogl::gl_debug(err))
		spdlog::error("GL error {}", err);
}

bool color_picker(const std::string& s, glm::vec4& color, unsigned int flag = 0) {
	ImGui::Text(s.c_str());
	ImGui::SameLine();
	return ImGui::ColorEdit3(s.c_str(), &color.x, flag | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
};

// https://www.shadertoy.com/view/ttc3zr
glm::uvec3 murmurHash32(glm::uvec2 src) {
	const glm::uint M = 0x5bd1e995u;
	glm::uvec3 h = glm::uvec3(1190494759u, 2147483647u, 3559788179u);
	src *= M; src ^= src >> 24u; src *= M;
	h *= M; h ^= src.x; h *= M; h ^= src.y;
	h ^= h >> 13u; h *= M; h ^= h >> 15u;
	return h;
}
glm::vec3 hash32(glm::vec2 src) {
	glm::uvec3 h = murmurHash32(floatBitsToUint(src));
	return glm::uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0f;
}

void gui_choice(GLenum& value, const std::string& name, const std::unordered_map<GLenum, std::string>& choices)
{
	if (ImGui::BeginCombo(name.c_str(), choices.at(value).c_str()))
	{
		for (const auto& choice : choices)
			if (ImGui::Selectable(choice.second.c_str(), choice.first == value) || ImGui::IsItemHovered())
				value = choice.first;

		ImGui::EndCombo();
	}
}

struct Window
{
	virtual void render_body(framework::RendererCollection& renderers) = 0;

	void render(framework::RendererCollection& renderers)
	{
		if (m_query)
		{
			GLint value = 0;
			m_query.get(value, GL_QUERY_RESULT_NO_WAIT);
			if (value) {
				m_current = (m_current + 1) % ValuesCount;
				m_values[m_current] = value * 1e-6f;
			}
		} else {
			m_query = picogl::Query::make(GL_TIME_ELAPSED);
		}

		m_query.begin();
		render_body(renderers);
		m_query.end();
	}

	void perf_gui()
	{
		if (ImGui::TreeNode("Perfs")) {
			std::array<float, ValuesCount> values;
			for (std::size_t i = 0; i < ValuesCount; ++i)
				values[i] = m_values[(m_current + i) % ValuesCount];
			ImGui::PlotLines("Render time", values.data(), ValuesCount, 0,
				fmt::format("{:1.1f}", m_values[(m_current - 1) % ValuesCount]).c_str(), 0.0f, 5.0f, ImVec2(250.0f, 50.0f));
			ImGui::TreePop();
		}
	}

	static constexpr std::size_t ValuesCount = 128;
	std::array<float, ValuesCount> m_values = {};
	std::size_t m_current = 0;
	picogl::Query m_query;
};

struct TexWindow : Window, framework::Viewport2D
{
	enum class Mode : GLenum
	{
		Checkers, Perlin, Kitten,
	};

	struct ModeData
	{
		ModeData(const std::string& name = "") : m_name{ name } {}
		ModeData(const ModeData& rhs) : m_name{ rhs.m_name } {}

		std::string m_name;
		picogl::Texture m_tex;
	};

	TexWindow() : framework::Viewport2D("Texture Viewer")
	{
	}

	void setup()
	{
		m_modes[Mode::Kitten].m_tex = framework::make_texture_from_file("../example/resources/kitten.png");
	}

	void settings_gui()
	{
		static const std::unordered_map<GLenum, std::string> wraps = {
			{ GL_REPEAT, "REPEAT" },
			{ GL_MIRRORED_REPEAT , "MIRRORED_REPEAT" },
			{ GL_CLAMP_TO_EDGE, "CLAMP_TO_EDGE" },
			{ GL_CLAMP_TO_BORDER, "CLAMP_TO_BORDER" },
		};

		static const std::unordered_map<GLenum, std::string> mag_filters = {
			{ GL_NEAREST , "NEAREST" },
			{ GL_LINEAR, "LINEAR" },
		};

		static const std::unordered_map<GLenum, std::string> min_filters = {
			{ GL_NEAREST_MIPMAP_NEAREST , "NEAREST_MIPMAP_NEAREST" },
			{ GL_LINEAR_MIPMAP_NEAREST , "LINEAR_MIPMAP_NEAREST" },
			{ GL_NEAREST_MIPMAP_LINEAR , "NEAREST_MIPMAP_LINEAR" },
			{ GL_LINEAR_MIPMAP_LINEAR , "LINEAR_MIPMAP_LINEAR" },
		};

		static const std::unordered_map<GLenum, std::string> channels = {
			{ GL_RED , "GL_RED" },
			{ GL_GREEN , "GL_GREEN" },
			{ GL_BLUE , "GL_BLUE" },
		};

		if (ImGui::BeginCombo("Texture", m_modes[m_mode].m_name.c_str()))
		{
			for (const auto& mode : m_modes)
				if (ImGui::Selectable(mode.second.m_name.c_str(), mode.first == m_mode) || ImGui::IsItemHovered())
					m_mode = mode.first;

			ImGui::EndCombo();
		}

		m_color_changed |= color_picker("first", m_color_A);
		ImGui::SameLine();
		m_color_changed |= color_picker("second", m_color_B);

		gui_choice(m_tex_wrap, "Wrapping", wraps);
		gui_choice(m_tex_mag_filter, "Mag Filter", mag_filters);
		gui_choice(m_tex_min_filter, "Min Filter", min_filters);

		ImGui::Text("Channel swizzling");
		for (int i = 0; i < 3; ++i) {
			const std::string nm = "##swizzling" + std::to_string(i);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(75);
			ImGui::SliderInt(nm.c_str(), &m_swizzle[i], GL_RED, GL_BLUE, channels.at(m_swizzle[i]).c_str());
		}

		ImGui::Checkbox("Force LoD", &m_force_lod);
		if (m_force_lod) {
			const picogl::Texture& tex = m_modes[m_mode].m_tex;
			ImGui::SameLine();
			ImGui::SetNextItemWidth(150);
			ImGui::SliderFloat("LoD", &m_lod, 0, static_cast<float>(tex.lod_count_2D()));
		}

		ImGui::Checkbox("Show closeup", &m_show_closeup);
		if (ImGui::Button("Reset")) {
			m_translation = glm::vec2(0.5f);
			m_scaling = glm::vec2(0.8f);
			m_angle = 1.5f;
			update();
		}

		perf_gui();
	}

	void gui_body()
	{
		framework::Viewport2D::gui_body();

		if (m_show_closeup && ImGui::IsItemHovered())
		{
			if (!m_readback_tex)
			{
				m_readback_img = framework::Image::make<glm::u8vec4>(readback_size, readback_size, 4);
				m_readback_tex = picogl::Texture::make_2d(GL_RGBA8, readback_size, readback_size);
				m_readback_tex.set_filtering(GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
			}

			const glm::vec2 image_size = { ImGui::GetItemRectSize().x, ImGui::GetItemRectSize().y };
			const glm::vec2 image_topleft = { ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y };
			const glm::vec2 mouse_pos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
			const glm::ivec2 tex_pos =
				glm::ivec2(glm::round(m_vp_size * (mouse_pos - image_topleft) / image_size)) - glm::ivec2(readback_radius);

			std::fill(m_readback_img.m_pixels.begin(), m_readback_img.m_pixels.end(), std::byte{ 0 });
			final_framebuffer().readback(m_readback_img.m_pixels.data(), tex_pos.x, tex_pos.y, readback_size, readback_size);
			m_readback_tex.upload_data(m_readback_img.m_pixels.data());

			ImGui::BeginTooltip();
			ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<std::size_t>(m_readback_tex)), { 150, 150 });
			ImGui::EndTooltip();
		}
	}

	void update() override
	{
		Viewport2D::update();

		if (m_color_changed)
		{
			m_modes[Mode::Checkers].m_tex = framework::make_texture_from_image(m_checkers, GL_RGBA32F);

			framework::Image perlin = (m_perlin * m_color_A).add<glm::vec4>((glm::vec4(1) - m_perlin) * m_color_B);
			m_modes[Mode::Perlin].m_tex = framework::make_texture_from_image(perlin, GL_RGBA32F);

			m_color_changed = false;
		}
	}

	picogl::Texture& get_texture()
	{
		return m_modes[m_mode].m_tex;
	}

	void render_body(framework::RendererCollection& renderers) override
	{
		if (!m_framebuffer)
			return;

		m_framebuffer.clear();
		glViewport(0, 0, m_framebuffer.width(), m_framebuffer.height());
		m_framebuffer.bind_draw();

		picogl::Texture& tex = get_texture();
		if (m_framebuffer && tex)
		{
			tex.set_filtering(m_tex_mag_filter, m_tex_min_filter);
			tex.set_wrapping(m_tex_wrap, m_tex_wrap);
			tex.set_swizzling(m_swizzle);
			tex.set_border_color(m_border_color);

			glDisable(GL_DEPTH_TEST);
			renderers.m_texture.render(m_modes[m_mode].m_tex, m_screen_to_uv, m_force_lod ? m_lod : -1.0f);
		}
	}

	static constexpr GLsizei readback_radius = 20;
	static constexpr GLsizei readback_size = 2 * readback_radius + 1;

	GLenum m_tex_wrap = GL_CLAMP_TO_BORDER;
	GLenum m_tex_mag_filter = GL_NEAREST;
	GLenum m_tex_min_filter = GL_NEAREST_MIPMAP_NEAREST;
	float m_lod = 0.0f;
	bool m_force_lod = false;
	bool m_show_closeup = true;

	std::array<int, 4> m_swizzle = { GL_RED , GL_GREEN, GL_BLUE, GL_ALPHA };
	std::array<float, 4> m_border_color = { 0.0f, 0.0f, 0.0f, 0.0f };

	glm::vec4 m_color_A = glm::vec4(0, 0, 0, 1);
	glm::vec4 m_color_B = glm::vec4(1);
	bool m_color_changed = true;

	framework::Image m_checkers = framework::make_checkers(50, 50, 5);
	framework::Image m_perlin = framework::make_perlin(150, 150, 5);
	framework::Image m_readback_img;
	picogl::Texture m_readback_tex;

	std::unordered_map<Mode, ModeData> m_modes = {
		{Mode::Kitten, ModeData{ "Kitten" }},
		{Mode::Checkers, ModeData{ "Checkers" }},
		{Mode::Perlin, ModeData{ "Perlin" }}
	};
	Mode m_mode = Mode::Kitten;
};

struct ModelerWindow : Window, framework::Viewport3D
{
	enum class RenderingMode : std::uint32_t { Phong, Point, Line, UVS, Colored, Textured };

	struct InstanceData
	{
		glm::mat4 m_object_to_world = {};
		glm::mat4 m_normal_to_world = {};
		GLint m_object_id = {};
		GLint m_instance_id = {};
		RenderingMode m_rendering_mode = {};
		std::int32_t pad = 0;
	};

	struct Mesh
	{
		picogl::Mesh m_gl_mesh = {};
		framework::AABB m_aabb = {};
		glm::mat4 m_self_transform = {};
	};

	struct Instance
	{
		glm::mat4 m_transform = glm::mat4(1);
		glm::vec4 m_color = glm::vec4(1, 0, 0, 1);
		RenderingMode m_rendering_mode = RenderingMode::Phong;
		float m_tessellation_level = 2.0f;
		float m_displacement_scaling = 1.0f;
		bool m_show_geometric_normals = false;
		bool m_show_vertex_normals = false;
		bool m_active_displacement = false;
		bool m_show_aabb = true;
		bool m_selected = true;
	};

	static Mesh make_mesh(framework::Mesh& mesh)
	{
		Mesh dst;
		dst.m_aabb = mesh.m_aabb;
		dst.m_gl_mesh = std::move(mesh.m_mesh);
		const glm::vec3 extent = dst.m_aabb.diagonal();
		const float max_extent = glm::max(glm::max(extent.x, extent.y), extent.z);
		dst.m_self_transform = glm::inverse(glm::translate(dst.m_aabb.center()) * glm::scale(glm::vec3(max_extent)));
		return dst;
	}

	static Mesh make_mesh_from_file(const std::string& path)
	{
		auto meshes = framework::make_mesh_from_obj(path);
		return make_mesh(meshes.front());
	}

	ModelerWindow() : framework::Viewport3D("Modeler", { GL_RGB32I })
	{
	}

	void update_instances()
	{
		m_instances_flatten.clear();

		const GLsizei object_count = m_combined_mesh.get_submeshes_count();
		m_instances_offset.resize(object_count, 0);

		std::size_t size = m_instances.empty() ? 0 : m_instances[0].size();
		for (std::size_t i = 1; i < object_count; ++i) {
			m_instances_offset[i] = m_instances_offset[i - 1] + m_instances_count[i - 1];
			size += m_instances[i].size();
		}
		m_instances_flatten.reserve(size);

		GLint object_id = 0;
		for (const auto& instances : m_instances) {
			GLint instance_id = 0;
			std::transform(instances.begin(), instances.end(), std::back_inserter(m_instances_flatten), [&](const Instance& i)
				{
					const Instance& instance = m_instances[object_id][instance_id];
					InstanceData o;
					o.m_object_id = object_id;
					o.m_instance_id = instance_id;
					o.m_object_to_world = instance.m_transform * m_meshes[object_id].m_self_transform;
					o.m_normal_to_world = glm::mat3(glm::transpose(glm::inverse(o.m_object_to_world)));
					o.m_rendering_mode = instance.m_rendering_mode;
					++instance_id;
					return o;
				});
			++object_id;
		}

		m_instance_ssbo = picogl::Buffer::make(GL_SHADER_STORAGE_BUFFER, m_instances_flatten);
		m_instance_offset_ssbo = picogl::Buffer::make(GL_SHADER_STORAGE_BUFFER, m_instances_offset);
	}

	void set_instances()
	{
		const GLsizei object_count = m_combined_mesh.get_submeshes_count();
		const int old_count = m_instances.empty() ? 0 : (int)m_instances[0].size();
		for (int object_id = 0; object_id < object_count; ++object_id) {
			if (m_instance_count == old_count)
				continue;

			m_instances_count[object_id] = m_instance_count;
			m_instances[object_id].resize(m_instance_count);
			for (int i = old_count; i < m_instance_count; ++i) {
				Instance& instance = m_instances[object_id][i];
				const glm::vec3 p = 20.0f * (hash32(glm::vec2(object_id / float(object_count), i / float(m_instance_count)) + 0.5f) - 0.5f);
				instance.m_transform =
					glm::translate(p) *
					glm::rotate(i * 0.3f, glm::vec3(1, 0, 0)) *
					m_instances[object_id].front().m_transform;
			}
		}
		m_combined_mesh.set_instances_count(m_instances_count);
		update_instances();
	}

	void setup()
	{
		m_camera.m_position = 3.0f * glm::vec3(1);

		m_meshes.push_back(make_mesh_from_file("../example/resources/apple.obj"));
		m_meshes.push_back(make_mesh_from_file("../example/resources/banana.obj"));
		m_meshes.push_back(make_mesh(framework::make_torus(1.0f, 0.4f, 32u)));

		m_combined_mesh = picogl::Mesh::combine({
			m_meshes[0].m_gl_mesh,
			m_meshes[1].m_gl_mesh,
			m_meshes[2].m_gl_mesh
			});

		const GLsizei object_count = m_combined_mesh.get_submeshes_count();
		m_instances_count.resize(object_count, 1);
		m_instances.resize(object_count);

		set_instances();
	}

	void settings_gui()
	{
		static const GLint max_sample_count = [] {
			GLint count;
			glGetIntegerv(GL_MAX_SAMPLES, &count);
			return count;
		}();

		ImGui::SliderInt("Sample Count", &m_sample_count, 1, max_sample_count);

		const GLsizei object_count = m_combined_mesh.get_submeshes_count();
		const int old_count = m_instance_count;
		if (ImGui::SliderInt("Instance Count", &m_instance_count, 1, 500))
			set_instances();

		static bool all = false;
		static int mode_all = 0;

		const GLuint selected_object = m_selected_instance.m_object_id - 1;
		const GLuint selected_instance = m_selected_instance.m_instance_id - 1;

		if (all) {
			if (ImGui::SliderInt("Rendering Mode", &mode_all, 0, 4)) {
				for (auto& instances : m_instances)
					for (auto& instance : instances)
						instance.m_rendering_mode = (RenderingMode)mode_all;
				update_instances();
			}
		} else if (selected_object < m_meshes.size() && selected_instance < m_instances[selected_object].size()) {
			Instance& instance = m_instances[selected_object][selected_instance];
			if (ImGui::SliderInt("Rendering Mode", reinterpret_cast<int*>(&instance.m_rendering_mode), 0, 4))
				update_instances();
		}
		ImGui::Checkbox("All", &all);
		if (ImGui::Button("Random")) {
			int i = 0;
			for (auto& instances : m_instances)
				for (auto& instance : instances)
					instance.m_rendering_mode = (RenderingMode)(5.0f * (0.5f + 0.5f * std::sin(123456.f * (++i))));
			update_instances();
		}

		perf_gui();
	}

	void gui_body() override
	{
		Viewport3D::gui_body();

		const glm::ivec2 mouse_position = glm::ivec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y) - glm::ivec2(m_vp_position);
		if (ImGui::IsWindowFocused() && ImGui::IsItemHovered()) {
			final_framebuffer().readback(&m_hovered_instance, mouse_position.x, mouse_position.y, 1, 1, GL_COLOR_ATTACHMENT1);
			if (m_hovered_instance.m_global_instance_id) {
				ImGui::BeginTooltip();
				ImGui::Text(fmt::format("Object {}, Instance {}", m_hovered_instance.m_object_id, m_hovered_instance.m_instance_id).c_str());
				ImGui::EndTooltip();
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left, false) && m_hovered_instance.m_global_instance_id)
				m_selected_instance = m_hovered_instance;
		}
	}

	void render_body(framework::RendererCollection& renderers) override
	{
		picogl::Framebuffer& fb = m_framebuffer;
		if (!fb)
			return;

		fb.clear<GLfloat>(GL_DEPTH, { 1.0f });
		fb.clear<GLfloat>(GL_COLOR, { 0.8f, 0.8f, 0.8f, 1.0f }, 0);
		fb.clear<GLint>(GL_COLOR, {}, 1);

		glViewport(0, 0, fb.width(), fb.height());

		fb.bind_draw();
		if (m_texture)
			m_texture->bind_as_sampler(GL_TEXTURE0);
		renderers.m_multi_renderer.render(m_camera, m_combined_mesh, m_instance_ssbo, m_instance_offset_ssbo);

		fb.bind_draw(GL_COLOR_ATTACHMENT0);
		if (m_selected_instance.m_global_instance_id) {
			const GLuint selected_object = m_selected_instance.m_object_id - 1;
			const GLuint selected_instance = m_selected_instance.m_instance_id - 1;
			if (selected_object < m_meshes.size() && selected_instance < m_instances[selected_object].size()) {
				const Instance& instance = m_instances[selected_object][selected_instance];
				const Mesh& mesh = m_meshes[selected_object];
				glLineWidth(2.0f);
				renderers.m_single_color.render(
					m_camera, framework::make_aabb_lines(mesh.m_aabb).m_mesh, instance.m_transform * mesh.m_self_transform, glm::vec4(0, 1, 0, 1));
			}
		}
		renderers.m_grid_renderer.render(m_camera);
	}

	std::vector<GLuint> m_instances_count;
	std::vector<std::vector<Instance>> m_instances;
	std::vector<InstanceData> m_instances_flatten;
	std::vector<GLint> m_instances_offset;
	picogl::Buffer m_instance_ssbo;
	picogl::Buffer m_instance_offset_ssbo;

	std::vector<Mesh> m_meshes;
	picogl::Mesh m_combined_mesh;
	const picogl::Texture* m_texture = {};

	int m_instance_count = 250;

	struct SelectedInstance
	{
		GLint m_object_id = 0;
		GLint m_instance_id = 0;
		GLint m_global_instance_id = 0;
	} m_hovered_instance, m_selected_instance;
};

struct RayMarchingWindow : Window, framework::Viewport3D
{
	RayMarchingWindow() : framework::Viewport3D("Raymarching")
	{
	}

	void setup()
	{
		m_cubemap = framework::make_cubemap_from_file("../example/resources/sky.png", GL_RGBA8);
		m_camera.m_position = 0.5f * glm::vec3(1, 0, 1);
		m_cube = framework::make_cube().m_mesh;

		auto vertex = picogl::Shader::make(GL_VERTEX_SHADER, framework::make_string_from_file(shader_path + "/mesh_interface.vert"));
		auto fragment = picogl::Shader::make(GL_FRAGMENT_SHADER, framework::make_string_from_file(shader_path + "/raymarching.frag"));
		m_raymarching = picogl::Program::make({ vertex, fragment });

		//Compute density.
		const int w = 64;
		std::vector<unsigned char> densities(w * w * w);
		for (int z = 0; z < w; ++z)
		{
			const float dz = z - w / 2.0f;
			for (int y = 0; y < w; ++y) {
				const float dy = y - w / 2.0f;
				for (int x = 0; x < w; ++x) {
					const float dx = x - w / 2.0f;
					const float s = 1.0f + 0.75f * framework::impl::make_random_vec<float, 1>().x;
					const float diff = glm::exp(-(dx * dx + dy * dy + dz * dz) / (2.0f * w));
					const float density = 255.0f * s * diff;
					const unsigned char d = static_cast<unsigned char>(glm::clamp(density, 0.0f, 255.0f));
					densities[x + w * (y + w * z)] = d;
				}
			}
		}
		m_density = picogl::Texture::make_3d(GL_R8, w, w, w, densities.data());
		m_density.set_wrapping(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
	}

	void settings_gui()
	{
		ImGui::SliderInt("Grid size", &m_grid_size, 1, 256);
		ImGui::SliderFloat("Intensity", &m_intensity, 2, 4);
		perf_gui();
	}

	void render_body(framework::RendererCollection& renderers) override
	{
		picogl::Framebuffer& fb = m_framebuffer;
		if (!fb)
			return;

		fb.clear();
		glViewport(0, 0, fb.width(), fb.height());

		fb.bind_draw();
		debug_gl();
		renderers.m_cubemap_renderer.render(m_camera, m_cubemap);
		{
			m_raymarching.use();
			m_density.bind_as_sampler(GL_TEXTURE0);
			m_raymarching.set_uniform("intensity", glUniform1f, m_intensity);
			m_raymarching.set_uniform("eye_pos", glUniform3fv, 1, glm::value_ptr(m_camera.m_position));
			m_raymarching.set_uniform("grid_size", glUniform3iv, 1, glm::value_ptr(glm::ivec3(m_grid_size)));
			m_raymarching.set_uniform("view_proj", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(m_camera.m_view_proj));
			m_raymarching.set_uniform("model", glUniformMatrix4fv, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
			m_cube.draw();
		}
		debug_gl();
	}

	picogl::Texture m_cubemap;
	picogl::Texture m_density;
	picogl::Mesh m_cube;
	picogl::Program m_raymarching;
	int m_grid_size = 256;
	float m_intensity = 3.0f;
};

struct DemoApp : framework::Application
{
	DemoApp(const std::filesystem::path& resource_path)
		: framework::Application("picoGL demo app"), m_resource_path{ resource_path }
	{
	}

	void setup() override
	{
		Application::setup();

		ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
		m_renderers = framework::RendererCollection::make(m_resource_path);
		m_tex_window.setup();
		m_modeler_window.setup();
		m_raymarching_window.setup();
	}

	void update() override
	{
		m_tex_window.update();
		m_modeler_window.update();
		m_raymarching_window.update();

		m_modeler_window.m_texture = &m_tex_window.get_texture();
	}

	void gui() override
	{
		glfwGetFramebufferSize(m_main_window.get(), &m_main_window_width, &m_main_window_height);
		if (m_main_window_width == 0 || m_main_window_height == 0)
			return;

		if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(m_main_window.get(), GLFW_TRUE);

		if (ImGui::Begin("Settings")) {
			if (ImGui::TreeNode("Texture Viewer"))
			{
				m_tex_window.settings_gui();
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Modeler"))
			{
				m_modeler_window.settings_gui();
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Raymarcher"))
			{
				m_raymarching_window.settings_gui();
				ImGui::TreePop();
			}
		}
		ImGui::End();

		m_tex_window.gui();
		m_modeler_window.gui();
		m_raymarching_window.gui();

		//ImGui::ShowDemoWindow();
	}

	void render() override
	{
		picogl::Framebuffer::get_default().clear();

		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		m_modeler_window.render(m_renderers);
		m_tex_window.render(m_renderers);
		m_raymarching_window.render(m_renderers);
	}

	framework::RendererCollection m_renderers;
	std::filesystem::path m_resource_path;

	TexWindow m_tex_window;
	ModelerWindow m_modeler_window;
	RayMarchingWindow m_raymarching_window;
};

int main(int argc, char* argv[])
{
	for (int i = 0; i < argc; ++i) {
		const std::string arg = argv[i];
		if (arg == "--shaders" && (i + 1) < argc)
			shader_path = argv[i + 1];
	}

	DemoApp app(shader_path);
	app.launch();

	return 0;
}