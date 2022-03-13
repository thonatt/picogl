#include <picogl/framework/application.h>

#include <spdlog/spdlog.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#define PICOGL_IMPLEMENTATION
#include <picogl/picogl.hpp>

namespace framework
{
	Application::Application(const std::string& name) : m_name{ name }
	{
	}

	void Application::setup()
	{
		if (!glfwInit())
			return;

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);

		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		if (!mode)
			return;

		const float desired_fps = 60.0;

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, static_cast<int>(std::round(mode->refreshRate / desired_fps)));

		const int w = mode->width, h = mode->height;
		m_main_window = std::shared_ptr<GLFWwindow>(glfwCreateWindow(w, h, m_name.c_str(), NULL, NULL), glfwDestroyWindow);

		if (!m_main_window)
			return;

		glfwMakeContextCurrent(m_main_window.get());
		glfwSwapInterval(static_cast<int>(std::round(mode->refreshRate / desired_fps)));

		if (!gladLoadGL())
			return;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(m_main_window.get(), false);
		ImGui_ImplOpenGL3_Init("#version 410");

		glfwSetMouseButtonCallback(m_main_window.get(), ImGui_ImplGlfw_MouseButtonCallback);
		glfwSetKeyCallback(m_main_window.get(), ImGui_ImplGlfw_KeyCallback);
		glfwSetCharCallback(m_main_window.get(), ImGui_ImplGlfw_CharCallback);
		glfwSetScrollCallback(m_main_window.get(), ImGui_ImplGlfw_ScrollCallback);

		auto debug_callback = [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
		{
			switch (severity) {
			default:
			case GL_DEBUG_SEVERITY_HIGH:
				spdlog::error("GL error: {}", message);
				break;
			case GL_DEBUG_SEVERITY_MEDIUM:
				spdlog::warn("GL warning: {}", message);
				break;
			case GL_DEBUG_SEVERITY_LOW:
			case GL_DEBUG_SEVERITY_NOTIFICATION:
				//spdlog::info("GL notification: {}", message);
				return;
			}
		};
		glDebugMessageCallback(debug_callback, nullptr);

		const unsigned char* renderer = glGetString(GL_RENDERER);
		const unsigned char* shading_langage_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
		spdlog::info("picoGL setup:");
		spdlog::info(" OpenGL version: {}.{}", GLVersion.major, GLVersion.minor);
		spdlog::info(" GPU: {}", (const char*)renderer);
		spdlog::info(" GLSL version: {}", (const char*)shading_langage_version);
	}

	void Application::launch()
	{
		setup();

		while (!glfwWindowShouldClose(m_main_window.get()))
		{
			glfwPollEvents();
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			update();

			render();

			gui();

			ImGui::Render();
			picogl::Framebuffer::get_default().bind_draw();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(m_main_window.get());
		}

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwTerminate();
	}
}