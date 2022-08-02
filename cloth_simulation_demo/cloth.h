#pragma once
#ifndef CLOTH_H_
#define CLOTH_H_

#define EIGEN_STACK_ALLOCATION_LIMIT 0

#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <utility>
#include <Eigen/dense>
#include <tbb/parallel_for.h>

using namespace Eigen;

static constexpr int spring_Y = 1e4;
static constexpr int dashpot_damping = 1e4;
static constexpr int drag_damping = 1;
static constexpr float fraction = 0.99f;
static constexpr float pi = 3.141592653589793f;

std::random_device rd;
std::mt19937 generator(rd());
std::uniform_real_distribution<float> dis(0., 1.0);

std::vector<std::pair<int, int>> spring_offset{ {-2,0},{-1,-1},{-1,0},{-1,1},{0,-2},{0,-1},{0,1},{0,2},{1,-1},{1,0},{1,1},{2,0} };

template<typename T>
using Vector3 = Matrix<T, 3, 1>;

template<typename T>
using Vector2 = Matrix<T, 2, 1>;

template<int M, int N, typename T = float>
class Cloth
{
public:
	Cloth(const T quad_size);
	~Cloth();

	void initialize();

public:
	Array<Vector3<T>, M, N> position;
	Array<Vector3<T>, M, N> velocity;
	T quad_size;
};


template<int Number, typename T = float>
class Balls
{
public:
	Balls(const T radius);
	~Balls();

	void initialize();

public:
	Array<Vector3<T>, Number, 1> center;
	T quad_size_ball;
	T radius;
};

template<int M, int N, typename T = float>
class Cloth_mesh
{
public:
	Cloth_mesh();
	~Cloth_mesh();

	void update_vertices(const Cloth<M, N, T>& cloth);

private:
	void update_triangles_normalvec(const Cloth<M, N, T>& cloth);

public:
	unsigned int* indices;
	T* vertices;

private:
	Array<Vector3<T>, Dynamic, Dynamic> bottom_left; //normal vector for triangle mesh
	Array<Vector3<T>, Dynamic, Dynamic> up_right; //normal vector for triangle mesh
};

template<int Number, int X_SEGMENTS = 30, int Y_SEGMENTS = 30, typename T = float>
class Balls_mesh
{
public:
	Balls_mesh();
	~Balls_mesh();

	void update_vertices(const Balls<Number, T>& balls);

public:
	unsigned int* indices;
	T* vertices;
};

template<int M, int N, typename T>
inline Cloth<M, N, T>::Cloth(const T quad_size)
{
	position = Array<Vector3<T>, M, N>::Zero();
	velocity = Array<Vector3<T>, M, N>::Zero();
	this->quad_size = quad_size;
}

template<int M, int N, typename T>
inline Cloth<M, N, T>::~Cloth()
{
}

template<int M, int N, typename T>
inline void Cloth<M, N, T>::initialize()
{
	T random_offset_x = 0.1 * (dis(generator) - 0.5);
	T random_offset_z = 0.1 * (dis(generator) - 0.5);

	tbb::parallel_for(tbb::blocked_range<int>(0, N), [&](const tbb::blocked_range<int>& r)
		{
			for (int j = r.begin(); j != r.end(); ++j)
			{
				for (int i = 0; i < M; ++i)
				{
					position.coeffRef(i, j).coeffRef(0) = i * quad_size - 0.5 + random_offset_x;
					position.coeffRef(i, j).coeffRef(1) = 0.6;
					position.coeffRef(i, j).coeffRef(2) = j * quad_size - 0.5 + random_offset_z;
					velocity(i, j) = Vector3<T>::Zero();
				}
			}
		}
	);
}

template<int Number, typename T>
inline Balls<Number, T>::Balls(const T radius)
{
	this->radius = radius;
	this->quad_size_ball = 0;
	center = Array<Vector3<T>, Number, 1>::Zero();
}

template<int Number, typename T>
inline Balls<Number, T>::~Balls()
{
}

template<int Number, typename T>
inline void Balls<Number, T>::initialize()
{
	quad_size_ball = 1.0 / Number;
	tbb::parallel_for(tbb::blocked_range<int>(0, Number), [&](const tbb::blocked_range<int>& r)
		{
			for (int i = r.begin(); i != r.end(); ++i)
			{
				center.coeffRef(i).coeffRef(0) = (i * quad_size_ball - 0.4 + (dis(generator) - 0.5) / 15) * 0.9;
				center.coeffRef(i).coeffRef(1) = ((dis(generator) - 0.5) / 3 - 0.1) * 0.9;
				center.coeffRef(i).coeffRef(2) = (i * quad_size_ball - 0.4 + (dis(generator) - 0.5) / 15) * 0.9;
			}
		}
	);
}


template<int M, int N, int Number, typename T=float>
void substep(Cloth<M, N, T>& cloth, const Balls<Number, T>& balls, const T dt)
{
	tbb::parallel_for(tbb::blocked_range<int>(0, N), [&](const tbb::blocked_range<int>& r)
		{
			for (int j = r.begin(); j != r.end(); ++j)
			{
				for (int i = 0; i < M; ++i)
				{
					Vector3<T> force(0., -9.8, 0.); //gravity
					for (auto& offset : spring_offset)
					{
						auto& [offset_i, offset_j] = offset;
						int another_i = i + offset_i;
						int another_j = j + offset_j;
						if (another_i >= 0 && another_i < M && another_j >= 0 && another_j < N)
						{
							Vector3<T> x_diff(cloth.position.coeff(i, j) - cloth.position.coeff(another_i, another_j));
							Vector3<T> v_diff(cloth.velocity.coeff(i, j) - cloth.velocity.coeff(another_i, another_j));
							T current_dist = x_diff.norm();
							Vector3<T> d(x_diff / current_dist);
							T original_dist = cloth.quad_size * (Vector2<T>(offset_i, offset_j).norm());

							force += (-spring_Y * d * (current_dist / original_dist - 1)); //spring force
							force += (-v_diff.dot(d) * d * dashpot_damping * cloth.quad_size); //dashpot damping
						}
					}

					cloth.velocity.coeffRef(i, j) += (force * dt);
				}
			}
		}
	);

	tbb::parallel_for(tbb::blocked_range<int>(0, N), [&](const tbb::blocked_range<int>& r)
		{
			for (int j = r.begin(); j != r.end(); ++j)
			{
				for (int i = 0; i < M; ++i)
				{
					cloth.velocity.coeffRef(i, j) *= std::expf(-drag_damping * dt);
					for (int k = 0; k < Number; ++k)  //handling collision with balls
					{
						Vector3<T> offset_to_center(cloth.position.coeff(i, j) - balls.center.coeff(k));
						if (offset_to_center.norm() <= balls.radius)
						{
							Vector3<T> normal(offset_to_center.normalized());
							cloth.velocity.coeffRef(i, j) -= (std::min(cloth.velocity.coeff(i, j).dot(normal), static_cast<T>(0)) * normal);
							cloth.velocity.coeffRef(i, j) *= fraction;
						}
					}

					cloth.position.coeffRef(i, j) += (cloth.velocity.coeffRef(i, j) * dt);
				}
			}
		}
	);
}


template<int M, int N, typename T>
inline Cloth_mesh<M, N, T>::Cloth_mesh() :bottom_left(M - 1, N - 1), up_right(M - 1, N - 1)
{
	int triangle_number = (M - 1) * (N - 1) * 2;
	indices = new unsigned int[triangle_number * 3];
	vertices = new T[M * N * 9]; // position, color and normal vector

	memset(vertices, 0x00, sizeof(vertices));
	tbb::parallel_for(tbb::blocked_range<int>(0, M-1), [&](const tbb::blocked_range<int>& r)
		{
			for (int i = r.begin(); i != r.end(); ++i)
			{
				for (int j = 0; j != N - 1; ++j)
				{
					int square_index = i * (N - 1) + j;
					int index = 6 * square_index;

					//first triangle
					indices[index + 0] = (i * N + j);
					indices[index + 1] = ((i + 1) * N + j);
					indices[index + 2] = (i * N + j + 1);

					//second triangle
					indices[index + 3] = ((i + 1) * N + j + 1);
					indices[index + 4] = (i * N + j + 1);
					indices[index + 5] = ((i + 1) * N + j);
				}
			}
		}
	);

	tbb::parallel_for(tbb::blocked_range<int>(0, N), [&](const tbb::blocked_range<int>& r)
		{
			for (int j = r.begin(); j != r.end(); ++j)
			{
				for (int i = 0; i < M; ++i)
				{
					int index = 9 * (i * N + j);
					if ((i / 4 + j / 4) % 2 == 0)
					{
						vertices[index + 3] = 0.0;
						vertices[index + 4] = 0.5;
						vertices[index + 5] = 1.0;
					}
					else
					{
						vertices[index + 3] = 1.0;
						vertices[index + 4] = 0.5;
						vertices[index + 5] = 0.0;
					}
				}
			}
		}
	);
}

template<int M, int N, typename T>
inline Cloth_mesh<M, N, T>::~Cloth_mesh()
{
	delete[] indices;
	delete[] vertices;
}

template<int M, int N, typename T>
inline void Cloth_mesh<M, N, T>::update_vertices(const Cloth<M, N, T>& cloth)
{
	update_triangles_normalvec(cloth);

	tbb::parallel_for(tbb::blocked_range<int>(0, N), [&](const tbb::blocked_range<int>& r)
		{
			for (int j = r.begin(); j != r.end(); ++j)
			{
				for (int i = 0; i < M; ++i)
				{
					int index = 9 * (i * N + j);
					Vector3<T> position_ij(cloth.position.coeff(i, j));

					//update position
					vertices[index + 0] = position_ij.x();
					vertices[index + 1] = position_ij.y();
					vertices[index + 2] = position_ij.z();

					//update normal vector
					//note that a vertex is joint with 6 triangles in our mesh
					Vector3<T> normal(Vector3<T>::Zero());
					if (i > 0 && i < M - 1 && j > 0 && j < N - 1)
					{
						normal = (bottom_left.coeff(i, j) + bottom_left.coeff(i - 1, j) + bottom_left.coeff(i, j - 1)
							+ up_right.coeff(i - 1, j) + up_right.coeff(i, j - 1) + up_right.coeff(i - 1, j - 1));
					}

					vertices[index + 6] = normal.x();
					vertices[index + 7] = normal.y();
					vertices[index + 8] = normal.z();
				}
			}
		}
	);
}

template<int M, int N, typename T>
inline void Cloth_mesh<M, N, T>::update_triangles_normalvec(const Cloth<M, N, T>& cloth)
{
	tbb::parallel_for(tbb::blocked_range<int>(0, N-1), [&](const tbb::blocked_range<int>& r)
		{
			for (int j = r.begin(); j != r.end(); ++j)
			{
				for (int i = 0; i < M-1; ++i)
				{
					Vector3<T> edge1(cloth.position.coeff(i + 1, j) - cloth.position.coeff(i, j));
					Vector3<T> edge2(cloth.position.coeff(i, j + 1) - cloth.position.coeff(i, j));
					Vector3<T> normal(edge1.cross(edge2));
					normal /= normal.norm();
					bottom_left.coeffRef(i, j) = normal;

					edge1 = cloth.position.coeff(i + 1, j) - cloth.position.coeff(i + 1, j + 1);
					edge2 = cloth.position.coeff(i, j + 1) - cloth.position.coeff(i + 1, j + 1);
					normal = edge1.cross(edge2);
					normal /= normal.norm();
					up_right.coeffRef(i, j) = normal;
				}
			}
		}
	);
}

template<int Number, int X_SEGMENTS, int Y_SEGMENTS, typename T>
inline Balls_mesh<Number, X_SEGMENTS, Y_SEGMENTS, T>::Balls_mesh()
{
	vertices = new T[Number * (X_SEGMENTS+1) * (Y_SEGMENTS+1) * 6]; // position and normal vector
	indices = new unsigned int[Number * X_SEGMENTS * Y_SEGMENTS * 6];

	for (int number = 0; number < Number; ++number)
	{
		int ball_index = number * X_SEGMENTS * Y_SEGMENTS * 6;
		int ball_index_of_vertices = number * (X_SEGMENTS + 1) * (Y_SEGMENTS + 1);
		tbb::parallel_for(tbb::blocked_range<int>(0, X_SEGMENTS), [&](const tbb::blocked_range<int>& r)
			{
				for (int i = r.begin(); i != r.end(); ++i)
				{
					for (int j = 0; j < Y_SEGMENTS; ++j)
					{
						int square_index = i * Y_SEGMENTS + j;
						int index = square_index * 6;
						indices[ball_index + index + 0] = ball_index_of_vertices + i * (Y_SEGMENTS + 1) + j;
						indices[ball_index + index + 1] = ball_index_of_vertices + (i + 1) * (Y_SEGMENTS + 1) + j;
						indices[ball_index + index + 2] = ball_index_of_vertices + i * (Y_SEGMENTS + 1) + j + 1;
						indices[ball_index + index + 3] = ball_index_of_vertices + (i + 1) * (Y_SEGMENTS + 1) + j + 1;
						indices[ball_index + index + 4] = ball_index_of_vertices + i * (Y_SEGMENTS + 1) + j + 1;
						indices[ball_index + index + 5] = ball_index_of_vertices + (i + 1) * (Y_SEGMENTS + 1) + j;
					}
				}
			}
		);
	}
}

template<int Number, int X_SEGMENTS, int Y_SEGMENTS, typename T>
inline Balls_mesh<Number, X_SEGMENTS, Y_SEGMENTS, T>::~Balls_mesh()
{
	delete [] indices;
	delete [] vertices;
}

template<int Number, int X_SEGMENTS, int Y_SEGMENTS, typename T>
inline void Balls_mesh<Number, X_SEGMENTS, Y_SEGMENTS, T>::update_vertices(const Balls<Number, T>& balls)
{
	T radius = balls.radius * 0.95;
	for (int number = 0; number < Number; ++number)
	{
		int ball_index_of_vertices = number * (X_SEGMENTS + 1) * (Y_SEGMENTS + 1) * 6;
		T center_x = balls.center.coeff(number).x();
		T center_y = balls.center.coeff(number).y();
		T center_z = balls.center.coeff(number).z();
		tbb::parallel_for(tbb::blocked_range<int>(0, X_SEGMENTS + 1), [&](const tbb::blocked_range<int>& r)
			{
				for (int i = r.begin(); i != r.end(); ++i)
				{
					T x_seg = static_cast<T>(i) / static_cast<T>(X_SEGMENTS);
					T cos_fi = std::cos(x_seg * 2 * pi);
					T sin_fi = std::sin(x_seg * 2 * pi);
					for (int j = 0; j < Y_SEGMENTS + 1; ++j)
					{
						T y_seg = static_cast<T>(j) / static_cast<T>(Y_SEGMENTS);
						int index = 6 * (i * (Y_SEGMENTS+1) + j);
						T sin_theta = std::sin(y_seg * pi);
						T cos_theta = std::cos(y_seg * pi);
						T normal_vec_x = sin_theta * cos_fi;
						T normal_vec_y = cos_theta;
						T normal_vec_z = sin_theta * sin_fi;
						vertices[ball_index_of_vertices + index + 0] = center_x + radius * normal_vec_x;
						vertices[ball_index_of_vertices + index + 1] = center_y + radius * normal_vec_y;
						vertices[ball_index_of_vertices + index + 2] = center_z + radius * normal_vec_z;
						vertices[ball_index_of_vertices + index + 3] = normal_vec_x;
						vertices[ball_index_of_vertices + index + 4] = normal_vec_y;
						vertices[ball_index_of_vertices + index + 5] = normal_vec_z;
					}
				}
			}
		);
	}
}

#endif