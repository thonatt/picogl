#pragma once

#define GLM_FORCE_RADIANS 
#include <glm/glm.hpp>

namespace framework
{
	class Camera
	{
	public:
		void update();

		glm::vec3 front() const;
		glm::vec3 right() const;
		glm::vec3 up() const;

		glm::mat4 m_view = {};
		glm::mat4 m_proj = {};
		glm::mat4 m_view_proj = {};
		glm::mat4 m_inverse_view = {};

		// Ray dir = x * rd[0] + y * rd[1] + rd[2] for x,y in [0,1]
		glm::mat3 m_ray_derivatives = {};
		glm::vec3 m_position = 1.5f * glm::vec3(1);
		glm::vec3 m_target = glm::vec3(0);
		glm::vec3 m_up = glm::vec3(0, 0, 1);
		float m_w = 1.0f;
		float m_h = 1.0f;
		float m_fov = glm::radians(60.0f);
		float m_near = 1e-2f;
		float m_far = 1e2f;
	};
}