#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <memory>

#include <picogl/picogl.hpp>

namespace framework
{
	class Application
	{

	public:
		Application(const std::string& name);

		virtual void setup();
		virtual void update() = 0;
		virtual void gui() = 0;
		virtual void render() = 0;

		void launch();

	protected:
		std::shared_ptr<GLFWwindow> m_main_window;
		int m_main_window_width = {};
		int m_main_window_height = {};
		std::string m_name = "myApp";
	};
}
