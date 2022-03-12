#include <picogl/framework/camera.h>

#include <glm/gtc/matrix_transform.hpp>

namespace framework
{
	void Camera::update()
	{
		const float aspect = m_w / m_h;
		m_proj = glm::perspective(m_fov, aspect, m_near, m_far);
		m_view = glm::lookAt(m_position, m_target, m_up);

		m_view_proj = m_proj * m_view;
		m_inverse_view = glm::inverse(m_view);

		const float h_world = 2.0f * glm::tan(m_fov / 2.0f);
		const float w_world = h_world * aspect;
		m_ray_derivatives[0] = w_world * right();
		m_ray_derivatives[1] = -h_world * up();
		m_ray_derivatives[2] = front() - (m_ray_derivatives[0] + m_ray_derivatives[1]) / 2.0f;
	}

	glm::vec3 Camera::front() const
	{
		return -m_inverse_view[2];
	}

	glm::vec3 Camera::right() const
	{
		return m_inverse_view[0];
	}

	glm::vec3 Camera::up() const
	{
		return m_inverse_view[1];
	}

}
