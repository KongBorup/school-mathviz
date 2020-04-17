#pragma once

#include <iostream>
#include <iomanip>
#include <exception>

struct InvalidDimensionsForMultiplication : public std::exception {};

template <size_t num_rows, size_t num_cols = 1>
class Vector_N;

template <size_t num_rows, size_t num_cols>
class Matrix_NxN
{
public:
    double mat[num_rows][num_cols];

    Matrix_NxN() {}

    Matrix_NxN(double inp_mat[num_rows][num_cols])
    {
        for (size_t row = 0; row < num_rows; row++)
        {
            for (size_t col = 0; col < num_cols; col++)
            {
                mat[row][col] = inp_mat[row][col];
            }
        }
    }

    // Prints a matrix to cout
    void print()
    {
        std::cout << std::fixed << std::setprecision(2);

        for (size_t row = 0; row < num_rows; row++)
        {
            std::cout << '[';
            for (size_t col = 0; col < (num_cols - 1); col++)
            {
                std::cout << mat[row][col] << "\t ";
            }
            std::cout << mat[row][num_cols - 1];
            std::cout << ']' << std::endl;
        }
    }

    // Multiplies two matrices that do not necessarily have the same dimensions.
    template <size_t other_num_rows, size_t other_num_cols>
    Matrix_NxN<num_rows, other_num_cols> mult(Matrix_NxN<other_num_rows, other_num_cols> other)
    {
        // Validate:
        //   - There MUST be the same number of columns in A as there are rows in
        //     B when they are multiplied as A*B.
        if (num_cols != other_num_rows)
        {
            throw InvalidDimensionsForMultiplication();
        }

        // Multiplying two matrices A*B will result in a new matrix with the same
        // number of rows as A and the same number of columns as B.
        double prod_arr[num_rows][other_num_cols] = {};
        Matrix_NxN<num_rows, other_num_cols> prod(prod_arr);

        for (size_t row = 0; row < num_rows; row++)
        {
            for (size_t other_col = 0; other_col < other_num_cols; other_col++)
            {
                for (size_t other_row = 0; other_row < other_num_rows; other_row++)
                {
                    prod.mat[row][other_col] += mat[row][other_row] * other.mat[other_row][other_col];
                }
            }
        }

        return prod;
    }

    // Adds two matrices together. They must have the same dimensions.
    Matrix_NxN<num_rows, num_cols> add(Matrix_NxN<num_rows, num_cols> other)
    {
        double sum_arr[num_rows][num_cols] = {};
        Matrix_NxN<num_rows, num_cols> sum(sum_arr);

        for (size_t row = 0; row < num_rows; row++)
        {
            for (size_t col = 0; col < num_cols; col++)
            {
                sum.mat[row][col] = mat[row][col] + other.mat[row][col];
            }
        }

        return sum;
    }

    // Multiplies by a scalar.
    Matrix_NxN<num_rows, num_cols> mult(double scalar)
    {
        double prod_arr[num_rows][num_cols] = {};
        Matrix_NxN<num_rows, num_cols> prod(prod_arr);

        for (size_t row = 0; row < num_rows; row++)
        {
            for (size_t col = 0; col < num_cols; col++)
            {
                prod.mat[row][col] = scalar * mat[row][col];
            }
        }

        return prod;
    }

    // Subtracts two matrices by adding the negative.
    Matrix_NxN<num_rows, num_cols> subtract(Matrix_NxN<num_rows, num_cols> other)
    {
        return this->add(other.mult(-1.0));
    }

    Vector_N<num_rows> to_vec()
    {
        double arr[num_rows];

        for (size_t row = 0; row < num_rows; row++)
        {
            arr[row] = mat[row][0];
        }

        Vector_N<num_rows> vec(arr);

        return vec;
    }
};

template <size_t num_rows, size_t num_cols = 1>
class Vector_N : public Matrix_NxN<num_rows, num_cols>
{
public:
    // Internally, we will represent vectors as a 1-column matrix.
    Vector_N(double inp_vec[num_rows])
    {
        for (size_t row = 0; row < num_rows; row++)
        {
            this->mat[row][0] = inp_vec[row];
        }
    }

    double dot(Vector_N<num_rows> other)
    {
        double sum = 0.0;
        for (size_t row = 0; row < num_rows; row++)
        {
            sum += this->mat[row][0] * other.mat[row][0];
        }
        return sum;
    }

    // Vector cross-product only makes sense in a 3-dimensional space.
    Vector_N<3> cross(Vector_N<3> other)
    {
        double arr[3] = {
            this->mat[1][0] * other.mat[2][0] - this->mat[2][0] * other.mat[1][0],
            this->mat[2][0] * other.mat[0][0] - this->mat[0][0] * other.mat[2][0],
            this->mat[0][0] * other.mat[1][0] - this->mat[1][0] * other.mat[0][0],
        };
        Vector_N<3> cp(arr);

        return cp;
    }

    // Assumes that the vector is at least 2-dimensional, which it will be in our use case.
    double get_x()
    {
        return this->mat[0][0];
    }

    // Assumes that the vector is at least 2-dimensional, which it will be in our use case.
    double get_y()
    {
        return this->mat[1][0];
    }
};
