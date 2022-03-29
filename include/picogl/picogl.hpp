#ifndef PICOGL_INCLUDE
#define PICOGL_INCLUDE

#include <array>
#include <string>
#include <utility>
#include <vector>

#ifndef PICOGL_ASSERT
#include <cassert>
#define PICOGL_ASSERT assert
#endif // !PICOGL_ASSERT

#define PICOGL_ENUM_CLASS_OPERATORS(Name)																						\
	constexpr Name operator&(const Name a, const Name b) {																		\
		return static_cast<Name>(static_cast<std::underlying_type_t<Name>>(a) & static_cast<std::underlying_type_t<Name>>(b));	\
	}																															\
	constexpr Name operator|(const Name a, const Name b) {																		\
		return static_cast<Name>(static_cast<std::underlying_type_t<Name>>(a) | static_cast<std::underlying_type_t<Name>>(b));	\
	}

namespace picogl
{
	namespace impl
	{
		struct PixelInfo
		{
			PixelInfo(GLenum internal_format,
				GLenum format,
				GLenum type,
				GLuint channel_count);

			GLenum m_internal_format;
			GLenum m_format;
			GLenum m_type;
			GLuint m_channel_count;
			GLuint m_scalar_sizeof;
		};

		PixelInfo get_pixel_info(const GLenum internal_format);
		GLuint get_scalar_sizeof(const GLenum type);

		template<typename Container>
		GLuint get_data_size(const Container& container);

		enum class GLObjectType
		{
			Buffer,
			Framebuffer,
			Query,
			Program,
			RenderBuffer,
			Shader,
			Texture,
			VertexArray
		};

		template<GLObjectType Type, typename ...Args>
		void gl_creator(GLuint* gl, Args ...args);

		template<GLObjectType Type>
		void gl_deleter(const GLuint* gl);

		template<GLObjectType Type>
		class GLObject
		{
		public:
			template<typename ...Args>
			static GLObject make(Args&& ...args);

			GLObject();
			GLObject(GLObject&& rhs) noexcept;
			GLObject& operator=(GLObject&& rhs) noexcept;

			operator GLuint() const;

			~GLObject();

		protected:
			GLuint m_id = 0;
		};
	}

	GLenum gl_debug(std::string& message);
	GLenum gl_framebuffer_status(std::string& message, const GLenum target = GL_FRAMEBUFFER);

	class Buffer
	{
	public:
		static Buffer make(
			const GLenum target,
			const GLsizeiptr size,
			const void* data = nullptr,
			const GLenum usage = GL_STATIC_DRAW);

		template<typename T>
		static Buffer make(const GLenum target, const std::vector<T>& values, const GLenum usage = GL_STATIC_DRAW);

		void copy_to(Buffer& dst, const GLintptr to = 0) const;
		void copy_to(Buffer& dst, const GLintptr to, const GLintptr from, const GLsizeiptr size) const;

		operator GLuint() const;
		void bind() const;
		void bind(const GLenum target) const;
		void bind_as_ssbo(const GLuint index) const;

		void upload_data(const void* data, const GLsizeiptr size = 0, const GLintptr offset = 0);
		GLsizeiptr get_size() const;

	private:
		impl::GLObject<impl::GLObjectType::Buffer> m_gl;
		GLenum m_target;
		GLsizeiptr m_size;
	};

	class Shader
	{
	public:
		static Shader make(const GLenum type, const std::string& code);

		bool compiled() const;
		const std::string& get_log() const;
		operator GLuint() const;

	private:
		impl::GLObject<impl::GLObjectType::Shader> m_gl;
		GLenum m_type;
		std::string m_log;
	};

	class Program
	{
	public:
		static Program make(const std::vector<std::reference_wrapper<const Shader>>& shaders);

		operator GLuint() const;
		bool linked() const;
		const std::string& get_log() const;

		void use() const;

		template<typename glUniformFunc, typename ...Args>
		void set_uniform(const char* name, glUniformFunc&& f, Args&& ...args) const;

	private:
		impl::GLObject<impl::GLObjectType::Program> m_gl;
		std::string m_log;
	};

	class Texture
	{
	public:
		enum class Options
		{
			AutomaticAlignment = 1 << 1,
			AllocateMipmap = 1 << 2,
			GenerateMipmap = AllocateMipmap | 1 << 3,
			FixedSampleLocations = 1 << 4,

			Default = AutomaticAlignment,
		};

		static Texture make_1d(
			const GLenum internal_format,
			const GLsizei width,
			const GLsizei array_size = 1,
			const void* data = nullptr,
			const Options opts = Options::Default);

		static Texture make_2d(
			const GLenum internal_format,
			const GLsizei width,
			const GLsizei height,
			const GLsizei array_size = 1,
			const GLsizei sample_count = 1,
			const void* data = nullptr,
			const Options opts = Options::Default);

		static Texture make_3d(
			const GLenum internal_format,
			const GLsizei width,
			const GLsizei height,
			const GLsizei depth,
			const void* data = nullptr,
			const Options opts = Options::Default);

		static Texture make_cubemap(
			const GLenum internal_format,
			const GLsizei width,
			const GLsizei height,
			const GLsizei array_size = 1,
			const Options opts = Options::Default
		);

		Texture& bind();
		const Texture& bind() const;
		GLsizei array_size() const;
		GLsizei sample_count() const;

		Texture& set_swizzling(const std::array<GLint, 4>& swizzle_mask = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA });
		Texture& set_wrapping(const GLenum s = GL_REPEAT, const GLenum t = GL_REPEAT, const GLenum r = GL_REPEAT);
		Texture& set_filtering(const GLenum mag_filer = GL_LINEAR, const GLenum min_filter = GL_NEAREST_MIPMAP_LINEAR);
		Texture& set_alignment(const GLint pack = 1, const GLint unpack = 1);
		Texture& set_border_color(const std::array<float, 4>& rgba);
		Texture& upload_data(const void* data, GLuint level = 0, GLuint layer = 0, GLenum face = 0);

		void bind_as_sampler(const GLuint slot = GL_TEXTURE0) const;
		void bind_as_image(
			const GLuint unit,
			const GLint level = 0,
			const GLint layer = 0,
			const GLenum access = GL_WRITE_ONLY) const;

		operator GLuint() const;
		GLenum get_format() const;
		GLenum get_target() const;
		GLenum get_type() const;
		GLenum get_internal_format() const;
		GLsizei width() const;
		GLsizei height() const;
		GLsizei depth() const;

		GLsizei lod_count_1D() const;
		GLsizei lod_count_2D() const;
		GLsizei lod_count_3D() const;

		void generate_mipmap() const;

	private:
		void make(
			const GLenum target,
			const GLenum internal_format,
			const GLsizei array_size,
			const GLsizei sample_count,
			const GLsizei width,
			const GLsizei height,
			const GLsizei depth,
			const void* data,
			const Options opts
		);

		impl::GLObject<impl::GLObjectType::Texture> m_gl;
		GLenum m_target = GL_TEXTURE_2D;
		GLenum m_internal_format;
		GLenum m_format;
		GLenum m_type;
		GLsizei m_array_size = 1;
		GLsizei m_sample_count = 1;
		GLsizei m_width = 0;
		GLsizei m_height = 0;
		GLsizei m_depth = 0;
		Options m_opts = Options::Default;
	};
	PICOGL_ENUM_CLASS_OPERATORS(Texture::Options);

	class Framebuffer
	{
	public:
		static Framebuffer make(const GLsizei width, const GLsizei height, const GLsizei sample_count = 1);
		static Framebuffer get_default(const GLsizei width = 0, const GLsizei height = 0, const GLsizei sample_count = 1);
		static Framebuffer make_from_texture(Texture&& texture);

		Framebuffer& set_depth_attachment(const GLenum format = GL_DEPTH_COMPONENT32);
		Framebuffer& add_color_attachment(const GLenum internal_format, const GLenum target = GL_TEXTURE_2D, Texture::Options options = {});

		void bind(const GLenum target = GL_FRAMEBUFFER) const;
		void bind_read(const GLenum attachment) const;
		void bind_draw(const GLenum attachment) const;
		void bind_draw() const;

		void clear(const std::array<float, 4>& rgba = { 0.0f, 0.0f, 0.0f, 1.0f }, GLenum mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) const;

		template<typename T>
		void clear(GLenum buffer, const std::array<T, 4>& rgba = {}, GLint attachement_index = 0) const;

		void readback(void* dst, GLint x, GLint y, GLsizei width, GLsizei height, GLenum attach_from = GL_COLOR_ATTACHMENT0) const;
		void readback(void* dst, GLenum attach_from = GL_COLOR_ATTACHMENT0) const;

		void blit_to(
			Framebuffer& to,
			const GLenum attachment_to = GL_COLOR_ATTACHMENT0,
			const GLenum filter = GL_NEAREST,
			const GLenum attachment_from = GL_COLOR_ATTACHMENT0) const;

		void blit_to(Framebuffer& to,
			GLint to_x,
			GLint to_y,
			GLint to_w,
			GLint to_h,
			const GLenum attachment_to,
			const GLenum filter,
			GLint from_x,
			GLint from_y,
			GLint from_w,
			GLint from_h,
			const GLenum attachment_from = GL_COLOR_ATTACHMENT0) const;

		GLsizei sample_count() const;
		GLsizei width() const;
		GLsizei height() const;
		const std::vector<Texture>& color_attachments() const;
		GLuint depth_handle() const;
		operator GLuint() const;

	private:
		impl::GLObject<impl::GLObjectType::Framebuffer> m_gl;
		impl::GLObject<impl::GLObjectType::RenderBuffer> m_depth_attachment;
		std::vector<Texture> m_color_attachments;
		std::vector<GLenum> m_attachments;
		GLsizei m_sample_count = 1;
		GLsizei m_width = 0;
		GLsizei m_height = 0;
	};

	class Mesh
	{
	public:
		struct VertexAttribute
		{
			template<typename Container>
			VertexAttribute(const Container& container, const GLenum type, const GLsizei channels, const bool normalized = false);

			GLenum m_type;
			const char* m_data;
			GLsizei m_channel_count;
			GLuint m_size;
			bool m_normalized = GL_FALSE;
		};

		static Mesh make();
		static Mesh combine(const std::vector<std::reference_wrapper<const Mesh>>& meshes);

		template<typename Container>
		Mesh& set_indices(const GLenum primitive_type, const Container& indices, const GLenum type = GL_UNSIGNED_INT);
		Mesh& set_vertex_attributes(const std::vector<VertexAttribute>& attributes);
		Mesh& set_instances_count(const std::vector<GLuint>& instances_count);

		void draw() const;
		void draw(GLenum primitive_type) const;
		void draw(GLenum primitive_type, GLsizei force_vertex_count) const;

		operator GLuint() const;
		GLsizei get_vertex_sizeof() const;
		GLsizei get_submeshes_count() const;

	private:
		struct SubMesh
		{
			GLuint m_index_count;
			GLuint m_indice_offset;
			GLuint m_first_index;
		};

		struct DrawElementsIndirectCommand
		{
			GLuint m_count;
			GLuint m_instance_count;
			GLuint m_first_index;
			GLuint m_base_vertex;
			GLuint m_base_instance;
		};

		void set_vertex_attribute(std::vector<char>& vertex_data, GLuint& index, std::size_t& offset,
			const GLsizei stride, const VertexAttribute& attribute);

		void setup_attribute_pointer(const GLuint index, const std::size_t offset, const GLsizei stride, const VertexAttribute& attribute);

		impl::GLObject<impl::GLObjectType::VertexArray> m_vao;
		Buffer m_index_buffer;
		Buffer m_vertex_buffer; // Vertex data is interleaved.
		Buffer m_indirect_draw_buffer;
		GLenum m_primitive_type = {};
		GLenum m_indice_type = {};
		GLsizei m_index_count = 0;
		GLsizei m_vertex_count = 0;
		std::vector<VertexAttribute> m_vertex_attributes;
		std::vector<SubMesh> m_submeshes;
		std::vector<GLuint> m_instances_count = { 1 };
	};

	class Query
	{
	public:
		static Query make(const GLenum target);

		void begin() const;
		void end() const;

		template<typename T>
		void get(T& t, const GLenum type = GL_QUERY_RESULT_NO_WAIT);

		operator GLuint() const;

	protected:
		impl::GLObject<impl::GLObjectType::Query> m_gl;
		GLenum m_target = {};
	};

	namespace impl
	{
		template<>
		inline void gl_creator<GLObjectType::Buffer>(GLuint* gl) {
			glCreateBuffers(1, gl);
		}

		template<>
		inline void gl_deleter<GLObjectType::Buffer>(const GLuint* gl) {
			glDeleteBuffers(1, gl);
		}

		template<>
		inline void gl_creator<GLObjectType::Framebuffer>(GLuint* gl) {
			glGenFramebuffers(1, gl);
		}

		template<>
		inline void gl_deleter<GLObjectType::Framebuffer>(const GLuint* gl) {
			glDeleteFramebuffers(1, gl);
		}

		template<>
		inline void gl_creator<GLObjectType::Query>(GLuint* gl) {
			glGenQueries(1, gl);
		}

		template<>
		inline void gl_deleter<GLObjectType::Query>(const GLuint* gl) {
			glDeleteQueries(1, gl);
		}

		template<>
		inline void gl_creator<GLObjectType::Program>(GLuint* gl) {
			*gl = glCreateProgram();
		}

		template<>
		inline void gl_deleter<GLObjectType::Program>(const GLuint* gl) {
			glDeleteProgram(*gl);
		}

		template<>
		inline void gl_creator<GLObjectType::RenderBuffer>(GLuint* gl) {
			glGenRenderbuffers(1, gl);
		}

		template<>
		inline void gl_deleter<GLObjectType::RenderBuffer>(const GLuint* gl) {
			glDeleteRenderbuffers(1, gl);
		}

		template<>
		inline void gl_creator<GLObjectType::Shader, GLenum>(GLuint* gl, const GLenum shader_type) {
			*gl = glCreateShader(shader_type);
		}

		template<>
		inline void gl_deleter<GLObjectType::Shader>(const GLuint* gl) {
			glDeleteShader(*gl);
		}

		template<>
		inline void gl_creator<GLObjectType::Texture>(GLuint* gl) {
			glGenTextures(1, gl);
		}

		template<>
		inline void gl_deleter<GLObjectType::Texture>(const GLuint* gl) {
			glDeleteTextures(1, gl);
		}

		template<>
		inline void gl_creator<GLObjectType::VertexArray>(GLuint* gl) {
			glGenVertexArrays(1, gl);
		}

		template<>
		inline void gl_deleter<GLObjectType::VertexArray>(const GLuint* gl) {
			glDeleteVertexArrays(1, gl);
		}

		template<GLObjectType Type>
		inline GLObject<Type>::GLObject() = default;

		template<GLObjectType Type>
		inline GLObject<Type>::GLObject(GLObject&& rhs) noexcept
		{
			std::swap(m_id, rhs.m_id);
		}

		template<GLObjectType Type>
		inline GLObject<Type>& GLObject<Type>::operator=(GLObject&& rhs) noexcept
		{
			std::swap(m_id, rhs.m_id);
			return *this;
		}

		template<GLObjectType Type>
		inline GLObject<Type>::operator GLuint() const
		{
			return m_id;
		}

		template<GLObjectType Type>
		inline GLObject<Type>::~GLObject()
		{
			if (m_id)
				gl_deleter<Type>(&m_id);
		}

		template<GLObjectType Type>
		template<typename ...Args>
		inline GLObject<Type> GLObject<Type>::make(Args && ...args)
		{
			GLObject<Type> obj;
			gl_creator<Type>(&obj.m_id, std::forward<Args>(args)...);
			return obj;
		}

		template<typename Container>
		GLuint get_data_size(const Container& container)
		{
			return GLuint(container.size() * sizeof(typename Container::value_type));
		}
	}

	template<typename Container>
	Mesh::VertexAttribute::VertexAttribute(const Container& container, const GLenum type, const GLsizei channels, const bool normalized)
		: m_data{ reinterpret_cast<const char*>(container.data()) }, m_channel_count{ channels }, m_type{ type }, m_normalized{ normalized }
	{
		constexpr std::size_t element_sizeof = sizeof(Container::value_type);
		PICOGL_ASSERT(element_sizeof% channels == 0);
		m_size = impl::get_data_size(container);
	}

	template<typename Container>
	Mesh& Mesh::set_indices(const GLenum primitive_type, const Container& indices, const GLenum type)
	{
		const GLuint type_sizeof = impl::get_scalar_sizeof(type);
		const GLuint indice_sizeof = sizeof(Container::value_type);

		PICOGL_ASSERT(m_vao);
		PICOGL_ASSERT(!indices.empty());
		PICOGL_ASSERT(indice_sizeof % type_sizeof == 0);

		m_index_buffer = Buffer::make(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);
		m_indice_type = type;
		m_primitive_type = primitive_type;
		m_index_count = static_cast<GLsizei>(indices.size() * (indice_sizeof / type_sizeof));

		SubMesh submesh;
		submesh.m_first_index = 0;
		submesh.m_index_count = static_cast<GLsizei>(indices.size() * (indice_sizeof / type_sizeof));
		submesh.m_indice_offset = 0;
		m_submeshes = { submesh };

		return *this;
	}
	template<typename T>
	inline Buffer Buffer::make(const GLenum target, const std::vector<T>& values, const GLenum usage)
	{
		return Buffer::make(target, impl::get_data_size(values), values.data(), usage);
	}

	template<typename T>
	void Framebuffer::clear(GLenum buffer, const std::array<T, 4>& rgba, GLint attachement_index) const
	{
		if (buffer == GL_DEPTH) {
			PICOGL_ASSERT((std::is_same_v<T, GLfloat>));
			bind();
			glClearBufferfv(buffer, 0, reinterpret_cast<const GLfloat*>(rgba.data()));
		} else {
			bind_draw(GL_COLOR_ATTACHMENT0 + attachement_index);
			if constexpr (std::is_same_v<T, GLfloat>)
				glClearBufferfv(buffer, 0, rgba.data());
			else if constexpr (std::is_same_v<T, GLint>)
				glClearBufferiv(buffer, 0, rgba.data());
			else if constexpr (std::is_same_v<T, GLuint>)
				glClearBufferiv(buffer, 0, rgba.data());
			else
				static_assert(false, "Not a GL supported type");
		}
	}

	template<typename T>
	inline void Query::get(T& t, const GLenum type)
	{
		if constexpr (std::is_same_v<T, GLint>)
			glGetQueryObjectiv(m_gl, type, &t);
		else if constexpr (std::is_same_v<T, GLuint>)
			glGetQueryObjectuiv(m_gl, type, &t);
		else if constexpr (std::is_same_v<T, GLint64>)
			glGetQueryObjecti64v(m_gl, type, &t);
		else if constexpr (std::is_same_v<T, GLuint64>)
			glGetQueryObjectui64v(m_gl, type, &t);
		else
			static_assert(false, "Not a GL supported type");
	}
}

#undef PICOGL_ENUM_CLASS_OPERATORS
#endif // !PICOGL_INCLUDE

#ifdef PICOGL_IMPLEMENTATION

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <type_traits>
#include <utility>

#define PICOGL_ENUM_STR(name) { name, #name }

namespace picogl
{
	namespace impl
	{
		inline PixelInfo::PixelInfo(GLenum internal_format, GLenum format, GLenum type, GLuint channel_count)
			: m_internal_format{ internal_format }, m_format{ format }, m_type{ type }, m_channel_count{ channel_count },
			m_scalar_sizeof{ get_scalar_sizeof(type) }
		{
		}

		inline PixelInfo get_pixel_info(const GLenum internal_format)
		{
			static const std::unordered_map<GLenum, PixelInfo> gl_pixel_infos =
			{
				{ GL_R8, { GL_R8, GL_RED, GL_UNSIGNED_BYTE, 1 } },
				{ GL_R32F, { GL_R32F, GL_RED, GL_FLOAT, 1 } },
				{ GL_R32I, { GL_R32I, GL_RED_INTEGER, GL_INT, 1 } },
				{ GL_RG32I, { GL_RG32I, GL_RG_INTEGER, GL_INT, 2 } },
				{ GL_RG32F, { GL_RG32F, GL_RG, GL_FLOAT, 2 } },
				{ GL_RGB8, { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, 3 } },
				{ GL_RGB32I, { GL_RGB32I, GL_RGB_INTEGER, GL_INT, 3 } },
				{ GL_RGB32F, { GL_RGB32F, GL_RGB, GL_FLOAT, 3 } },
				{ GL_RGBA8, { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4 } },
				{ GL_RGBA32F, { GL_RGBA32F, GL_RGBA, GL_FLOAT, 4 } },
			};

			return gl_pixel_infos.at(internal_format);
		}

		inline GLuint get_scalar_sizeof(const GLenum type)
		{
			static const std::unordered_map<GLenum, GLuint> gl_scalar_type_sizeofs = {
				{ GL_BYTE, 1 },
				{ GL_UNSIGNED_BYTE, 1 },
				{ GL_SHORT, 2 },
				{ GL_UNSIGNED_SHORT, 2 },
				{ GL_INT, 4 },
				{ GL_UNSIGNED_INT, 4 },
				{ GL_FIXED, 4 },
				{ GL_HALF_FLOAT, 2 },
				{ GL_FLOAT, 4 },
				{ GL_DOUBLE, 8 },
			};

			return gl_scalar_type_sizeofs.at(type);
		}
	}

	inline GLenum gl_debug(std::string& message)
	{
		static const std::unordered_map<GLenum, std::string> errors = {
			PICOGL_ENUM_STR(GL_INVALID_ENUM),
			PICOGL_ENUM_STR(GL_INVALID_VALUE),
			PICOGL_ENUM_STR(GL_INVALID_OPERATION),
			PICOGL_ENUM_STR(GL_STACK_OVERFLOW),
			PICOGL_ENUM_STR(GL_STACK_UNDERFLOW),
			PICOGL_ENUM_STR(GL_OUT_OF_MEMORY),
			PICOGL_ENUM_STR(GL_INVALID_FRAMEBUFFER_OPERATION)
		};

		const GLenum gl_err = glGetError();
		if (gl_err) {
			const auto it = errors.find(gl_err);
			if (it != errors.end())
				message = it->second;
		}
		return gl_err;
	}

	inline GLenum picogl::gl_framebuffer_status(std::string& message, const GLenum target)
	{
		static const std::unordered_map<GLenum, std::string> errors = {
			PICOGL_ENUM_STR(GL_FRAMEBUFFER_UNDEFINED),
			PICOGL_ENUM_STR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT),
			PICOGL_ENUM_STR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT),
			PICOGL_ENUM_STR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER),
			PICOGL_ENUM_STR(GL_FRAMEBUFFER_UNSUPPORTED),
			PICOGL_ENUM_STR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE),
			PICOGL_ENUM_STR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
		};

		GLenum status = glCheckFramebufferStatus(target);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			const auto it = errors.find(status);
			if (it != errors.end())
				message = it->second;
		}
		return status;
	}

	inline Buffer Buffer::make(const GLenum target, const GLsizeiptr size, const void* data, const GLenum usage)
	{
		Buffer buffer;
		buffer.m_gl = impl::GLObject<impl::GLObjectType::Buffer>::make();
		buffer.m_target = target;
		buffer.m_size = size;
		buffer.bind();
		glBufferData(target, size, data, usage);
		return buffer;
	}

	inline void Buffer::copy_to(Buffer& dst, const GLintptr to) const
	{
		copy_to(dst, to, 0, m_size);
	}

	inline void Buffer::copy_to(Buffer& dst, const GLintptr to, const GLintptr from, const GLsizeiptr size) const
	{
		bind(GL_COPY_READ_BUFFER);
		dst.bind(GL_COPY_WRITE_BUFFER);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, from, to, size);
	}

	inline Buffer::operator GLuint() const
	{
		return m_gl;
	}

	inline void Buffer::bind() const
	{
		bind(m_target);
	}

	inline void Buffer::bind(const GLenum target) const
	{
		glBindBuffer(target, m_gl);
	}

	inline void Buffer::bind_as_ssbo(const GLuint index) const
	{
		bind(GL_SHADER_STORAGE_BUFFER);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_gl);
	}

	inline void Buffer::upload_data(const void* data, const GLsizeiptr size, const GLintptr offset)
	{
		bind();
		glBufferSubData(m_target, offset, size ? size : m_size, data);
	}

	inline GLsizeiptr Buffer::get_size() const
	{
		return m_size;
	}

	inline Shader Shader::make(const GLenum type, const std::string& code)
	{
		Shader shader;
		shader.m_gl = impl::GLObject<impl::GLObjectType::Shader>::make(type);

		const char* code_ptr = code.c_str();
		glShaderSource(shader, 1, &code_ptr, NULL);
		glCompileShader(shader);

		GLint log_length, compile_status = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
		if (!compile_status) {
			shader.m_log.resize(static_cast<std::size_t>(log_length) + 1);
			glGetShaderInfoLog(shader, log_length, NULL, shader.m_log.data());
		}

		return shader;
	}

	inline bool Shader::compiled() const
	{
		return !m_log.empty();
	}

	inline const std::string& Shader::get_log() const
	{
		return m_log;
	}

	inline Shader::operator GLuint() const
	{
		return m_gl;
	}

	inline Program Program::make(const std::vector<std::reference_wrapper<const Shader>>& shaders)
	{
		Program program;
		program.m_gl = impl::GLObject<impl::GLObjectType::Program>::make();

		for (const auto& shader : shaders)
			glAttachShader(program, shader.get());

		glLinkProgram(program.m_gl);

		GLint log_length, link_status = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &link_status);
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
		if (!link_status) {
			program.m_log.resize(static_cast<std::size_t>(log_length) + 1);
			glGetProgramInfoLog(program, log_length, NULL, program.m_log.data());
		}

		for (const auto& shader : shaders)
			glDetachShader(program, shader.get());

		return program;
	}

	inline Program::operator GLuint() const
	{
		return m_gl;
	}

	inline bool Program::linked() const
	{
		return !m_log.empty();
	}

	inline const std::string& Program::get_log() const
	{
		return m_log;
	}

	inline void Program::use() const
	{
		glUseProgram(m_gl);
	}

	template<typename glUniformFunc, typename ...Args>
	inline void Program::set_uniform(const char* name, glUniformFunc&& f, Args && ...args) const
	{
		const GLint location = glGetUniformLocation(m_gl, name);
		f(location, std::forward<Args>(args)...);
	}

	inline Texture Texture::make_1d(const GLenum internal_format, const GLsizei width, const GLsizei array_size, const void* data, const Options opts)
	{
		Texture texture;
		const GLenum target = (array_size > 1 ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D);
		texture.make(target, internal_format, array_size, 1, width, 0, 0, data, opts);
		return texture;
	}

	inline Texture Texture::make_2d(const GLenum internal_format, const GLsizei width, const GLsizei height, const GLsizei array_size, const GLsizei sample_count, const void* data, const Options opts)
	{
		Texture texture;
		const GLenum target = sample_count > 1 ?
			(array_size > 1 ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_MULTISAMPLE) :
			(array_size > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D);
		texture.make(target, internal_format, array_size, sample_count, width, height, 0, data, opts);
		return texture;
	}

	inline Texture Texture::make_3d(const GLenum internal_format, const GLsizei width, const GLsizei height, const GLsizei depth, const void* data, const Options opts)
	{
		Texture texture;
		texture.make(GL_TEXTURE_3D, internal_format, 1, 1, width, height, depth, data, opts);
		return texture;
	}

	inline Texture Texture::make_cubemap(const GLenum internal_format, const GLsizei width, const GLsizei height, const GLsizei array_size, const Options opts)
	{
		Texture texture;
		const GLenum target = (array_size > 1 ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP);
		texture.make(target, internal_format, array_size, 1, width, height, 0, nullptr, opts);
		return texture;
	}

	inline GLsizei Texture::array_size() const
	{
		return m_array_size;
	}

	inline GLsizei Texture::sample_count() const
	{
		return m_sample_count;
	}

	inline Texture& Texture::bind()
	{
		glBindTexture(m_target, m_gl);
		return *this;
	}

	inline const Texture& Texture::bind() const
	{
		glBindTexture(m_target, m_gl);
		return *this;
	}

	inline Texture& Texture::set_swizzling(const std::array<GLint, 4>& swizzle_mask)
	{
		bind();
		glTexParameteriv(m_target, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask.data());
		return *this;
	}

	inline Texture& Texture::set_wrapping(const GLenum s, const GLenum t, const GLenum r)
	{
		bind();
		glTexParameteri(m_target, GL_TEXTURE_WRAP_S, s);
		glTexParameteri(m_target, GL_TEXTURE_WRAP_T, t);
		if (m_depth > 1)
			glTexParameteri(m_target, GL_TEXTURE_WRAP_R, r);
		return *this;
	}

	inline Texture& Texture::set_filtering(const GLenum mag_filer, const GLenum min_filter)
	{
		bind();
		glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, mag_filer);
		glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, min_filter);
		return *this;
	}

	inline Texture& Texture::set_alignment(const GLint pack, const GLint unpack)
	{
		bind();
		glPixelStorei(GL_PACK_ALIGNMENT, pack);
		glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
		return *this;
	}

	inline Texture& Texture::set_border_color(const std::array<float, 4>& rgba)
	{
		bind();
		glTexParameterfv(m_target, GL_TEXTURE_BORDER_COLOR, rgba.data());
		return *this;
	}

	inline Texture& Texture::upload_data(const void* data, GLuint level, GLuint layer, GLenum face)
	{
		bind();
		switch (m_target)
		{
		case GL_TEXTURE_1D:
			glTexSubImage1D(m_target, level, 0, m_width, m_format, m_type, data);
			break;
		case GL_TEXTURE_1D_ARRAY:
			glTexSubImage2D(m_target, level, 0, layer, m_width, 1, m_format, m_type, data);
			break;
		case GL_TEXTURE_2D:
			glTexSubImage2D(m_target, level, 0, 0, m_width, m_height, m_format, m_type, data);
			break;
		case GL_TEXTURE_CUBE_MAP:
			glTexSubImage2D(face, level, 0, 0, m_width, m_height, m_format, m_type, data);
			break;
		case GL_TEXTURE_2D_ARRAY:
			glTexSubImage3D(m_target, level, 0, 0, layer, m_width, m_height, 1, m_format, m_type, data);
			break;
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			glTexSubImage3D(face, level, 0, 0, layer, m_width, m_height, 1, m_format, m_type, data);
			break;
		case GL_TEXTURE_3D:
			glTexSubImage3D(m_target, level, 0, 0, 0, m_width, m_height, m_depth, m_format, m_type, data);
			break;
		default:
			break;
		}
		return *this;
	}

	inline void Texture::bind_as_sampler(const GLuint slot) const
	{
		PICOGL_ASSERT(slot >= GL_TEXTURE0);
		bind();
		glActiveTexture(slot);
	}

	inline void Texture::bind_as_image(const GLuint unit, const GLint level, const GLint layer, const GLenum access) const
	{
		glBindImageTexture(unit, m_gl, level, (m_array_size > 1), layer, access, m_internal_format);
	}

	inline Texture::operator GLuint() const
	{
		return m_gl;
	}

	inline GLenum Texture::get_format() const
	{
		return m_format;
	}

	inline GLenum Texture::get_target() const
	{
		return m_target;
	}

	inline GLenum Texture::get_type() const
	{
		return m_type;
	}

	inline GLenum Texture::get_internal_format() const
	{
		return m_internal_format;
	}

	inline GLsizei Texture::width() const
	{
		return m_width;
	}

	inline GLsizei Texture::height() const
	{
		return m_height;
	}

	inline GLsizei Texture::depth() const
	{
		return m_depth;
	}

	inline void Texture::make(
		const GLenum target,
		const GLenum internal_format,
		const GLsizei array_size,
		const GLsizei sample_count,
		const GLsizei width,
		const GLsizei height,
		const GLsizei depth,
		const void* data,
		const Options opts)
	{
		const impl::PixelInfo pixel_info = impl::get_pixel_info(internal_format);

		m_gl = impl::GLObject<impl::GLObjectType::Texture>::make();
		m_target = target;
		m_internal_format = internal_format;
		m_format = pixel_info.m_format;
		m_type = pixel_info.m_type;
		m_array_size = array_size;
		m_sample_count = sample_count;
		m_width = width;
		m_height = height;
		m_depth = depth;
		m_opts = opts;

		bind();

		if (bool(m_opts & Options::AutomaticAlignment))
		{
			if ((pixel_info.m_scalar_sizeof * m_width) % 4 != 0) {
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			}
		}

		const GLboolean fixed_locations = bool(m_opts & Options::FixedSampleLocations);
		const bool allocate_mipmap = bool(m_opts & Options::AllocateMipmap);
		switch (m_target) {
		case GL_TEXTURE_1D:
		{
			const GLsizei lod_count = allocate_mipmap ? lod_count_1D() : 1;
			glTexStorage1D(m_target, lod_count, m_internal_format, m_width);
			if (data)
				glTexSubImage1D(m_target, 0, 0, m_width, m_format, m_type, data);
			break;
		}
		case GL_TEXTURE_1D_ARRAY:
		{
			const GLsizei lod_count = allocate_mipmap ? lod_count_1D() : 1;
			glTexStorage2D(m_target, lod_count, m_internal_format, m_width, m_array_size);
			break;
		}
		case GL_TEXTURE_2D:
		case GL_TEXTURE_CUBE_MAP:
		{
			const GLsizei lod_count = allocate_mipmap ? lod_count_2D() : 1;
			glTexStorage2D(m_target, lod_count, m_internal_format, m_width, m_height);
			if (data)
				glTexSubImage2D(m_target, 0, 0, 0, m_width, m_height, m_format, m_type, data);
			break;
		}
		case GL_TEXTURE_2D_MULTISAMPLE:
		{
			glTexStorage2DMultisample(m_target, m_sample_count, m_internal_format, m_width, m_height, fixed_locations);
			break;
		}
		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_CUBE_MAP_ARRAY:
		{
			const GLsizei lod_count = allocate_mipmap ? lod_count_2D() : 1;
			glTexStorage3D(m_target, lod_count, m_internal_format, m_width, m_height, m_array_size);
			break;
		}
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		{
			glTexStorage3DMultisample(m_target, m_sample_count, m_internal_format, m_width, m_height, m_array_size, fixed_locations);
			break;
		}
		case GL_TEXTURE_3D:
		{
			const GLsizei lod_count = allocate_mipmap ? lod_count_3D() : 1;
			glTexStorage3D(m_target, lod_count, m_internal_format, m_width, m_height, m_depth);
			if (data)
				glTexSubImage3D(m_target, 0, 0, 0, 0, m_width, m_height, m_depth, m_format, m_type, data);
			break;
		}
		default:
			PICOGL_ASSERT(false);
			break;
		}

		if (bool(opts & Options::GenerateMipmap))
			glGenerateMipmap(m_target);

		std::string str;
		gl_debug(str);
	}

	inline GLsizei Texture::lod_count_1D() const
	{
		return static_cast<GLsizei>(std::floor(std::log2(m_width))) + 1;
	}

	inline GLsizei Texture::lod_count_2D() const
	{
		return static_cast<GLsizei>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
	}

	inline GLsizei Texture::lod_count_3D() const
	{
		return static_cast<GLsizei>(std::floor(std::log2(std::max({ m_width, m_height, m_depth })))) + 1;
	}

	inline void Texture::generate_mipmap() const
	{
		bind();
		glGenerateMipmap(m_target);
	}

	inline Framebuffer Framebuffer::make(const GLsizei width, const GLsizei height, const GLsizei sample_count)
	{
		Framebuffer framebuffer;
		framebuffer.m_gl = impl::GLObject<impl::GLObjectType::Framebuffer>::make();
		framebuffer.m_sample_count = sample_count;
		framebuffer.m_width = width;
		framebuffer.m_height = height;
		return framebuffer;
	}

	inline Framebuffer Framebuffer::get_default(const GLsizei width, const GLsizei height, const GLsizei sample_count)
	{
		Framebuffer fb;
		fb.m_sample_count = sample_count;
		fb.m_width = width;
		fb.m_height = height;
		return fb;
	}

	inline Framebuffer& Framebuffer::set_depth_attachment(const GLenum format)
	{
		m_depth_attachment = impl::GLObject<impl::GLObjectType::RenderBuffer>::make();
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth_attachment);
		if (m_sample_count > 1)
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_sample_count, format, m_width, m_height);
		else
			glRenderbufferStorage(GL_RENDERBUFFER, format, m_width, m_height);

		bind();
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_attachment);
		return *this;
	}

	inline Framebuffer& Framebuffer::add_color_attachment(const GLenum internal_format, const GLenum target, Texture::Options options)
	{
		const GLenum attachment_index = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(m_color_attachments.size());
		if (m_depth_attachment)
			options = options | Texture::Options::FixedSampleLocations | Texture::Options::AutomaticAlignment;

		m_attachments.push_back(attachment_index);
		switch (target)
		{
		default:
		case GL_TEXTURE_2D:
			m_color_attachments.push_back(Texture::make_2d(internal_format, m_width, m_height, 1, m_sample_count, nullptr, options));
			break;
		case GL_TEXTURE_CUBE_MAP:
			m_color_attachments.push_back(Texture::make_cubemap(internal_format, m_width, m_height, 1, options));
			break;
		}

		bind();
		const Texture& attachment = m_color_attachments.back();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_index, attachment.get_target(), attachment, 0);
		return *this;
	}

	inline void Framebuffer::bind(const GLenum target) const
	{
		glBindFramebuffer(target, m_gl);
	}

	inline void Framebuffer::bind_read(const GLenum attachment) const
	{
		//PICOGL_ASSERT(attachment >= GL_COLOR_ATTACHMENT0);
		bind(GL_READ_FRAMEBUFFER);
		glReadBuffer(attachment);
	}

	inline void Framebuffer::bind_draw(const GLenum attachment) const
	{
		PICOGL_ASSERT(attachment >= GL_COLOR_ATTACHMENT0);
		bind(GL_DRAW_FRAMEBUFFER);
		if (m_gl)
			glDrawBuffers(1, &attachment);
	}

	inline void Framebuffer::bind_draw() const
	{
		bind(GL_DRAW_FRAMEBUFFER);
		if (m_gl) {
			PICOGL_ASSERT(!m_attachments.empty());
			glDrawBuffers(static_cast<GLsizei>(m_attachments.size()), m_attachments.data());
		}
	}

	inline void Framebuffer::clear(const std::array<float, 4>& rgba, GLenum mask) const
	{
		bind_draw();
		glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
		glClear(mask);
	}

	inline void Framebuffer::readback(void* dst, GLint x, GLint y, GLsizei width, GLsizei height, GLenum attach_from) const
	{
		bind_read(attach_from);
		const Texture& attachment = m_color_attachments[attach_from - GL_COLOR_ATTACHMENT0];
		glReadPixels(x, y, width, height, attachment.get_format(), attachment.get_type(), dst);
	}

	inline void Framebuffer::readback(void* dst, GLenum attach_from) const
	{
		readback(dst, 0, 0, m_width, m_height, attach_from);
	}

	inline void Framebuffer::blit_to(Framebuffer& to, const GLenum attachment_to, const GLenum filter, const GLenum attachment_from) const
	{
		blit_to(to, 0, 0, to.m_width, to.m_height, attachment_to, filter, 0, 0, m_width, m_height, attachment_from);
	}

	inline void Framebuffer::blit_to(Framebuffer& to, GLint to_x, GLint to_y, GLint to_w, GLint to_h, const GLenum attachment_to, const GLenum filter, GLint from_x, GLint from_y, GLint from_w, GLint from_h, const GLenum attachment_from) const
	{
		bind_read(attachment_from);
		to.bind_draw(attachment_to);
		glBlitFramebuffer(from_x, from_y, from_w, from_h, to_x, to_y, to_w, to_h, GL_COLOR_BUFFER_BIT, filter);
	}

	inline GLsizei Framebuffer::sample_count() const
	{
		return m_sample_count;
	}

	inline GLsizei Framebuffer::width() const
	{
		return m_width;
	}

	inline GLsizei Framebuffer::height() const
	{
		return m_height;
	}

	inline const std::vector<Texture>& Framebuffer::color_attachments() const
	{
		return m_color_attachments;
	}

	inline GLuint Framebuffer::depth_handle() const
	{
		return m_depth_attachment;
	}

	inline Framebuffer::operator GLuint() const
	{
		return m_gl;
	}

	inline Mesh Mesh::make()
	{
		Mesh mesh;
		mesh.m_vao = impl::GLObject<impl::GLObjectType::VertexArray>::make();
		return mesh;
	}

	inline Mesh Mesh::combine(const std::vector<std::reference_wrapper<const Mesh>>& meshes)
	{
		PICOGL_ASSERT(meshes.size() > 0);
		const GLuint attribute_count = GLuint(meshes.front().get().m_vertex_attributes.size());
		PICOGL_ASSERT(attribute_count > 0);

		Mesh dst = Mesh::make();

		std::size_t total_submesh_count = 0;
		std::size_t total_instance_count = 0;
		GLsizeiptr vertex_buffer_size = 0;
		GLsizeiptr index_buffer_size = 0;

		for (const auto& mesh_ref : meshes) {
			const Mesh& mesh = mesh_ref.get();

			total_submesh_count += mesh.m_submeshes.size();
			dst.m_vertex_count += mesh.m_vertex_count;
			vertex_buffer_size += mesh.m_vertex_buffer.get_size();

			dst.m_index_count += mesh.m_index_count;
			index_buffer_size += mesh.m_index_buffer.get_size();

			total_instance_count += mesh.m_instances_count.size();

			if (!dst.m_primitive_type)
				dst.m_primitive_type = mesh.m_primitive_type;
			else
				PICOGL_ASSERT(dst.m_primitive_type == mesh.m_primitive_type);

			if (!dst.m_indice_type)
				dst.m_indice_type = mesh.m_indice_type;
			else
				PICOGL_ASSERT(dst.m_indice_type == mesh.m_indice_type);
		}

		dst.m_index_buffer = Buffer::make(GL_ELEMENT_ARRAY_BUFFER, index_buffer_size);
		dst.m_vertex_buffer = Buffer::make(GL_ARRAY_BUFFER, vertex_buffer_size);

		// Setup attribs pointers.
		glBindVertexArray(dst.m_vao);
		dst.m_vertex_buffer.bind();

		GLsizei attributes_sizeof = 0;
		for (const Mesh::VertexAttribute& attribute : meshes.front().get().m_vertex_attributes)
			attributes_sizeof += attribute.m_channel_count * impl::get_scalar_sizeof(attribute.m_type);

		GLsizeiptr attribute_offset = 0;
		for (GLuint index = 0; index < attribute_count; ++index) {
			const Mesh::VertexAttribute& attribute = meshes.front().get().m_vertex_attributes[index];
			dst.setup_attribute_pointer(index, attribute_offset, attributes_sizeof, attribute);
			attribute_offset += attribute.m_channel_count * impl::get_scalar_sizeof(attribute.m_type);
		}

		// Combine submeshes and gpu-copy buffers.
		GLsizeiptr dst_index_size_offset = 0;
		GLsizeiptr dst_vertex_size_offset = 0;
		GLsizei index_offset = 0;
		GLsizei index_count = 0;
		std::size_t global_submesh_id = 0;
		dst.m_instances_count.resize(total_instance_count);
		dst.m_submeshes.resize(total_submesh_count);
		for (const auto& mesh_ref : meshes) {
			const Mesh& mesh = mesh_ref.get();

			std::size_t local_submesh_id = 0;
			for (const SubMesh& src_submesh : mesh.m_submeshes)
			{
				SubMesh& submesh = dst.m_submeshes[global_submesh_id];
				submesh.m_first_index = index_count + src_submesh.m_first_index;
				submesh.m_index_count = src_submesh.m_index_count;
				submesh.m_indice_offset = src_submesh.m_indice_offset + index_offset;

				dst.m_instances_count[global_submesh_id] = mesh.m_instances_count[local_submesh_id];
				++local_submesh_id;
				++global_submesh_id;
			}

			const GLsizeiptr indices_size = mesh.m_index_buffer.get_size();
			mesh.m_index_buffer.copy_to(dst.m_index_buffer, dst_index_size_offset);

			dst_index_size_offset += indices_size;
			index_offset += mesh.m_vertex_count;
			index_count += mesh.m_index_count;

			mesh.m_vertex_buffer.copy_to(dst.m_vertex_buffer, dst_vertex_size_offset);
			dst_vertex_size_offset += mesh.m_vertex_buffer.get_size();
		}

		dst.set_instances_count(dst.m_instances_count);
		return dst;
	}

	inline Mesh& Mesh::set_vertex_attributes(const std::vector<VertexAttribute>& attributes)
	{
		PICOGL_ASSERT(m_vao);
		m_vertex_attributes = attributes;

		GLsizeiptr total_size = 0;
		GLsizei attributes_sizeof = 0;
		for (const VertexAttribute& attribute : attributes)
		{
			PICOGL_ASSERT(attribute.m_size > 0);
			total_size += attribute.m_size;
			attributes_sizeof += attribute.m_channel_count * impl::get_scalar_sizeof(attribute.m_type);
		}
		m_vertex_buffer = Buffer::make(GL_ARRAY_BUFFER, total_size);

		std::vector<char> vertex_data(total_size);
		GLuint index = 0;
		std::size_t offset = 0;
		glBindVertexArray(m_vao);
		m_vertex_buffer.bind();
		for (const VertexAttribute& attribute : attributes)
			set_vertex_attribute(vertex_data, index, offset, attributes_sizeof, attribute);

		m_vertex_buffer.upload_data(vertex_data.data());
		return *this;
	}

	inline Mesh& Mesh::set_instances_count(const std::vector<GLuint>& instances_count)
	{
		PICOGL_ASSERT(instances_count.size() == get_submeshes_count());
		m_instances_count = instances_count;

		std::vector<DrawElementsIndirectCommand> draws(m_submeshes.size());
		std::size_t mesh_id = 0;
		for (const SubMesh& submesh : m_submeshes) {
			DrawElementsIndirectCommand& draw = draws[mesh_id];

			draw.m_count = submesh.m_index_count;
			draw.m_instance_count = instances_count[mesh_id];
			draw.m_first_index = submesh.m_first_index;
			draw.m_base_vertex = submesh.m_indice_offset;
			draw.m_base_instance = 0;

			++mesh_id;
		}

		m_indirect_draw_buffer = Buffer::make(GL_DRAW_INDIRECT_BUFFER, draws);
		return *this;
	}

	inline void Mesh::set_vertex_attribute(std::vector<char>& vertex_data, GLuint& index, std::size_t& offset, const GLsizei stride, const VertexAttribute& attribute)
	{
		const std::size_t scalar_sizeof = impl::get_scalar_sizeof(attribute.m_type);
		const std::size_t attribute_sizeof = scalar_sizeof * attribute.m_channel_count;
		PICOGL_ASSERT(attribute.m_size % attribute_sizeof == 0);

		const char* src = attribute.m_data;
		char* dst = vertex_data.data() + offset;
		const char* dst_end = vertex_data.data() + vertex_data.size();
		while (dst < dst_end) {
			std::memcpy(dst, src, attribute_sizeof);
			src += attribute_sizeof;
			dst += stride;
		}

		setup_attribute_pointer(index, offset, stride, attribute);

		const GLsizei vertex_count = static_cast<GLsizei>(attribute.m_size / attribute_sizeof);
		if (!m_vertex_count)
			m_vertex_count = vertex_count;
		else
			PICOGL_ASSERT(vertex_count == m_vertex_count);

		offset += attribute_sizeof;
		++index;
	}

	inline void Mesh::setup_attribute_pointer(const GLuint index, const std::size_t offset, const GLsizei stride, const VertexAttribute& attribute)
	{
		if (attribute.m_type == GL_FLOAT)
			glVertexAttribPointer(index, attribute.m_channel_count, attribute.m_type, attribute.m_normalized, stride, ((char*)0) + offset);
		else if (attribute.m_type == GL_INT || attribute.m_type == GL_UNSIGNED_INT)
			glVertexAttribIPointer(index, attribute.m_channel_count, attribute.m_type, stride, ((char*)0) + offset);
		else
			PICOGL_ASSERT(false);

		glEnableVertexAttribArray(index);
	}

	inline void Mesh::draw() const
	{
		draw(m_primitive_type);
	}

	inline void Mesh::draw(GLenum primitive_type) const
	{
		PICOGL_ASSERT(m_vao);
		glBindVertexArray(m_vao);
		if (m_index_buffer) {
			m_index_buffer.bind();
			if (m_indirect_draw_buffer) {
				m_indirect_draw_buffer.bind();
				glMultiDrawElementsIndirect(primitive_type, m_indice_type, 0, GLsizei(m_submeshes.size()), 0);
			} else
				glDrawElements(primitive_type, m_index_count, m_indice_type, 0);
		} else
			glDrawArrays(primitive_type, 0, m_index_count);
	}

	inline void Mesh::draw(GLenum primitive_type, GLsizei force_vertex_count) const
	{
		PICOGL_ASSERT(m_vao);
		glBindVertexArray(m_vao);
		glDrawArrays(primitive_type, 0, force_vertex_count);
	}

	inline Mesh::operator GLuint() const
	{
		return m_vao;
	}

	inline GLsizei Mesh::get_vertex_sizeof() const
	{
		GLsizei vertex_sizeof = 0;
		for (const VertexAttribute& attribute : m_vertex_attributes)
			vertex_sizeof += attribute.m_channel_count * impl::get_scalar_sizeof(attribute.m_type);
		return vertex_sizeof;
	}

	inline GLsizei Mesh::get_submeshes_count() const
	{
		return GLsizei(m_submeshes.size());
	}

	inline Query Query::make(const GLenum target)
	{
		Query query;
		query.m_gl = impl::GLObject<impl::GLObjectType::Query>::make();
		query.m_target = target;
		return query;
	}

	inline void Query::begin() const
	{
		glBeginQuery(m_target, m_gl);
	}

	inline void Query::end() const
	{
		glEndQuery(m_target);
	}

	inline Query::operator GLuint() const
	{
		return m_gl;
	}
}

#undef PICOGL_ENUM_STR
#endif // PICOGL_IMPLEMENTATION
