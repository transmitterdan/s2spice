//
// This is slightly modified version of public available QSMatix class,
// described in these articles:
// https://www.quantstart.com/articles/Matrix-Classes-in-C-The-Header-File
// https://www.quantstart.com/articles/Matrix-Classes-in-C-The-Source-File
//
// License terms:
//
// Copyright © 2012-2017 Michael Halls-Moore
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Changes from the original code presented in the article:
// - renamed QSMatrix data members
// - removed unnecesary "this->" before member access
// - optimized constructors and assignment operators
// - added move constructor and assignment operator
// - using "std::size_t" instead of "unsigned" for the matrix dimensions
// - modified meber operator() signature to eliminate passing primitive type
//   by reference
// - added new methods at() to enable access with checking dimension boundaries
// - fixed error in the method transpose(), which worked incorrectly with
//   non-square matrices
// - modified member operator*(const XQSMatrix&) to validate matrix dimensions.
// - added new static method identity() for generation of identity matrix
// - added new method inverse() to compute of inverse matrix
// - added new static method readCsv() to construct matrix from data in the
//   CSV file
// - added new methods set_row_count() and set_col_count() to resize matrix
// - added method window() to create new matrix from rectangular window
//   in the current matrix
// - added new method swap() and used it in some other methods to optimize
//   computation speed
// - added new method add_columns() to inject new columns size_to matrix
// - added new method remove_columns() to remove columns from matrix
// - added new methods mul_by_row() to multiply this matrix by vector,
//   which represents single row matrix
// - added new methods mul_by_column() to multiply this matrix by vector,
//   which represents single column matrix
// - added new method row_scalar_product() to find scalar product of matix row
//   with a given vector
// - added new method column_scalar_product() to find scalar product of matix
//   column
//   with a given vector
// - added new method row() to extract row as matrix
// - added new method row_as_vector() to extract row as vector
// - added new method column() to extract column as matrix
// - added new method column_as_vector() to extract column as vector
// - added stream output operator
// - class renamed to XQSMatrix
// - added operator[] and new versions of method at() to access individual rows
//
// Changes are maintained on the GitHub:
// https://github.com/ivanp2015/xqsmatrix
//
// License terms for modifications:
//
// Copyright © 2015-2017 Ivan Pizhenko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef XQSMATRIX_H
#define XQSMATRIX_H

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

template <typename T>
class XQSMatrix {
private:
  std::size_t m_nrows;
  std::size_t m_ncols;
  std::vector<std::vector<T>> m_mat;

  // Check that matrix has equal dimensions
  void check_equal_dimensions(const XQSMatrix<T>& other) const;
  // Check that matrix has dimensions that are suitable for product oeration
  void check_suitable_for_product(const XQSMatrix<T>& other) const;
  // Validate row index
  void validate_row_index(std::size_t index) const;
  // Validate column index
  void validate_column_index(std::size_t index) const;

  // Tokenize string and parse tokens as matrix cell values.
  template <class Converter>
  static std::vector<T> tokenizeAndParse(const std::string& str,
                                         const Converter& converter,
                                         const std::string& delimiters = " ",
                                         bool trimEmpty = false);

  // Helper function for gaussian reduction.
  // Used to find inverse matrix.
  std::vector<size_t> gaussian_reduction();

public:
  // Constructors
  explicit XQSMatrix(std::size_t nrows = 1, std::size_t ncols = 1);
  XQSMatrix(std::size_t nrows, std::size_t ncols, const T& v);
  XQSMatrix(const XQSMatrix& src);
  XQSMatrix(XQSMatrix&& src);

  // Destructor
  virtual ~XQSMatrix() {}

  // Create identity matrix with K on the diaginal
  static XQSMatrix<T> identity(std::size_t n, const T& k = T(1));

  // Swap matrices
  void swap(XQSMatrix& other) noexcept;

  // Operator overloading, for "standard" mathematical matrix operations
  XQSMatrix& operator=(const XQSMatrix<T>& rhs);
  XQSMatrix& operator=(XQSMatrix<T>&& rhs);

  // Matrix mathematical operations
  XQSMatrix operator+(const XQSMatrix<T>& rhs) const;
  XQSMatrix& operator+=(const XQSMatrix<T>& rhs);
  XQSMatrix operator-(const XQSMatrix<T>& rhs) const;
  XQSMatrix& operator-=(const XQSMatrix<T>& rhs);
  XQSMatrix operator*(const XQSMatrix<T>& rhs) const;
  XQSMatrix& operator*=(const XQSMatrix<T>& rhs);
  XQSMatrix transpose() const;
  XQSMatrix inverse_v1() const;
  XQSMatrix inverse_v2() const;

  // Matrix/scalar operations
  XQSMatrix operator+(const T& rhs) const;
  XQSMatrix operator-(const T& rhs) const;
  XQSMatrix operator*(const T& rhs) const;
  XQSMatrix operator/(const T& rhs) const;
  XQSMatrix& operator+=(const T& rhs);
  XQSMatrix& operator-=(const T& rhs);
  XQSMatrix& operator*=(const T& rhs);
  XQSMatrix& operator/=(const T& rhs);

  // Matrix/vector operations
  std::vector<T> diag_vec() const;
  // Multiple by vector as row
  XQSMatrix mul_by_row(const std::vector<T>& row_data) const;
  // Multiple by vector as column
  std::vector<T> mul_by_column(const std::vector<T>& column_data) const;
  // Scalar product of the row with a given vector
  T row_scalar_product(std::size_t row_index, const std::vector<T>& v) const;
  // Scalar product of the column with a given vector
  T column_scalar_product(std::size_t col_index, const std::vector<T>& v) const;

  // Add "count" columns at postion "pos" with inital value "v"
  void add_columns(std::size_t pos, std::size_t count, const T& v);
  // Remove "count" columns at postion "pos"
  void remove_columns(std::size_t pos, std::size_t count);

  // Access the rows
  std::vector<T>& operator[](std::size_t i) noexcept { return m_mat[i]; }

  const std::vector<T>& operator[](std::size_t i) const noexcept {
    return m_mat[i];
  }

  std::vector<T>& at(std::size_t i) { return m_mat.at(i); }

  const std::vector<T>& at(std::size_t i) const { return m_mat.at(i); }

  // Access the individual elements
  T& operator()(std::size_t row, std::size_t col) noexcept {
    return m_mat[row][col];
  }

  const T& operator()(std::size_t row, std::size_t col) const noexcept {
    return m_mat[row][col];
  }

  T& at(std::size_t row, std::size_t col) { return m_mat.at(row).at(col); }

  const T& at(std::size_t row, std::size_t col) const {
    return m_mat.at(row).at(col);
  }

  // Access the row and column sizes
  std::size_t row_count() const noexcept { return m_nrows; }

  std::size_t col_count() const noexcept { return m_ncols; }

  // Change row and colums sizes
  void set_row_count(std::size_t new_rows);
  void set_col_count(std::size_t new_cols);

  // Extact rectangular window as new matrix
  XQSMatrix window(std::size_t row, std::size_t col, std::size_t nrows,
                   std::size_t ncols) const;
  // Extract matrix row as matrix
  XQSMatrix<T> row(std::size_t index) const;
  // Extract matrix row as vector
  std::vector<T> row_as_vector(std::size_t index) const;
  // Extract matrix column as matrix
  XQSMatrix<T> column(std::size_t index) const;
  // Extract matrix column as vector
  std::vector<T> column_as_vector(std::size_t index) const;

  // Create from CSV file
  template <class Converter>
  static XQSMatrix readCsv(const std::string& path, char lineEnding,
                           const std::string& fieldDelimiters,
                           const Converter& converter,
                           std::size_t numberOfHeaderLines);

  const std::vector<std::vector<T>>& inner_vector() const noexcept {
    return m_mat;
  }
};

// Parameter Constructor
template <typename T>
XQSMatrix<T>::XQSMatrix(std::size_t nrows, std::size_t ncols)
    : m_nrows(nrows), m_ncols(ncols), m_mat(m_nrows) {
  for (std::size_t i = 0; i < nrows; i++) {
    m_mat[i].resize(ncols);
  }
}

template <typename T>
XQSMatrix<T>::XQSMatrix(std::size_t nrows, std::size_t ncols, const T& v)
    : m_nrows(nrows), m_ncols(ncols), m_mat(m_nrows) {
  for (std::size_t i = 0; i < nrows; i++) {
    m_mat[i].resize(ncols, v);
  }
}

// Copy Constructor
template <typename T>
XQSMatrix<T>::XQSMatrix(const XQSMatrix<T>& src)
    : m_nrows(src.m_nrows), m_ncols(src.m_ncols), m_mat(src.m_mat) {}

// Move Constructor
template <typename T>
XQSMatrix<T>::XQSMatrix(XQSMatrix<T>&& src)
    : m_nrows(src.m_nrows), m_ncols(src.m_ncols), m_mat(std::move(src.m_mat)) {}

template <typename T>
XQSMatrix<T> XQSMatrix<T>::identity(std::size_t n, const T& k) {
  XQSMatrix result(n, n, 0);
  for (std::size_t i = 0; i < n; ++i) result.m_mat[i][i] = k;
  return result;
}

// Swap matrices
template <typename T>
inline void XQSMatrix<T>::swap(XQSMatrix<T>& other) noexcept {
  std::swap(m_nrows, other.m_nrows);
  std::swap(m_ncols, other.m_ncols);
  m_mat.swap(other.m_mat);
}

// Assignment Operator
template <typename T>
inline XQSMatrix<T>& XQSMatrix<T>::operator=(const XQSMatrix<T>& rhs) {
  if (&rhs == this) {
    m_mat = rhs.m_mat;
    m_nrows = rhs.m_nrows;
    m_ncols = rhs.m_ncols;
  }
  return *this;
}

// Move Assignment Operator
template <typename T>
XQSMatrix<T>& XQSMatrix<T>::operator=(XQSMatrix<T>&& rhs) {
  if (&rhs != this) {
    std::vector<std::vector<T>> tmp(1);
    tmp[0].resize(1);
    tmp.swap(rhs.m_mat);
    m_mat.swap(tmp);
    m_nrows = rhs.m_nrows;
    m_ncols = rhs.m_ncols;
    rhs.m_nrows = 1;
    rhs.m_ncols = 1;
  }
  return *this;
}

// Addition of two matrices
template <typename T>
inline XQSMatrix<T> XQSMatrix<T>::operator+(const XQSMatrix<T>& rhs) const {
  check_equal_dimensions(rhs);
  XQSMatrix result(m_nrows, m_ncols);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    const auto& orow = rhs.m_mat[i];
    auto& rrow = result.m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      rrow[j] = row[j] + orow[j];
    }
  }
  return result;
}

// Cumulative addition of this matrix and another
template <typename T>
XQSMatrix<T>& XQSMatrix<T>::operator+=(const XQSMatrix<T>& rhs) {
  check_equal_dimensions(rhs);
  for (std::size_t i = 0; i < m_nrows; i++) {
    auto& row = m_mat[i];
    const auto& orow = rhs.m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      row[j] += orow[j];
    }
  }
  return *this;
}

// Subtraction of this matrix and another
template <typename T>
inline XQSMatrix<T> XQSMatrix<T>::operator-(const XQSMatrix<T>& rhs) const {
  check_equal_dimensions(rhs);
  XQSMatrix result(m_nrows, m_ncols);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    const auto& orow = rhs.m_mat[i];
    auto& rrow = result.m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      rrow[j] = row[j] - orow[j];
    }
  }
  return result;
}

// Cumulative subtraction of this matrix and another
template <typename T>
XQSMatrix<T>& XQSMatrix<T>::operator-=(const XQSMatrix<T>& rhs) {
  check_equal_dimensions(rhs);
  for (std::size_t i = 0; i < m_nrows; i++) {
    auto& row = m_mat[i];
    const auto& orow = rhs.m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      row[j] -= orow[j];
    }
  }
  return *this;
}

// Left multiplication of this matrix and another
template <typename T>
XQSMatrix<T> XQSMatrix<T>::operator*(const XQSMatrix<T>& rhs) const {
  check_suitable_for_product(rhs);
  const auto ncols = rhs.col_count();
  XQSMatrix result(m_nrows, ncols, 0.0);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    for (std::size_t j = 0; j < ncols; j++) {
      auto& res = result.m_mat[i][j];
      for (std::size_t k = 0; k < m_ncols; k++) {
        res += row[k] * rhs.m_mat[k][j];
      }
    }
  }
  return result;
}

// Cumulative left multiplication of this matrix and another
template <typename T>
inline XQSMatrix<T>& XQSMatrix<T>::operator*=(const XQSMatrix<T>& rhs) {
  XQSMatrix result = (*this) * rhs;
  swap(result);
  return *this;
}

// Calculate a transpose of this matrix
template <typename T>
XQSMatrix<T> XQSMatrix<T>::transpose() const {
  XQSMatrix result(m_ncols, m_nrows);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      result.m_mat[j][i] = row[j];
    }
  }
  return result;
}

// Calculate an inverse of this matrix (version #1)
// based on the ideas from
// http://www.sanfoundry.com/java-program-find-inverse-matrix/
template <typename T>
XQSMatrix<T> XQSMatrix<T>::inverse_v1() const {
  if (m_nrows != m_ncols) {
    throw std::logic_error("Can't invert non-square matrix");
  }

  const std::size_t N = m_nrows;
  XQSMatrix a(*this);
  auto index = a.gaussian_reduction();

  // Update the matrix b[i][j] with the ratios stored
  XQSMatrix b = identity(N);
  for (std::size_t i = 0; i < N - 1; ++i) {
    for (std::size_t j = i + 1; j < N; ++j) {
      const auto& av = a.m_mat[index[j]][i];
      for (std::size_t k = 0; k < N; ++k) {
        b.m_mat[index[j]][k] -= av * b.m_mat[index[i]][k];
      }
    }
  }

  // Perform backward substitutions
  XQSMatrix x(N, N);
  auto& xrow = x.m_mat[N - 1];
  auto& arow = a.m_mat[index[N - 1]];
  auto& brow = b.m_mat[index[N - 1]];
  const auto& aa = arow[N - 1];
  if (aa == 0) {
    throw std::runtime_error("Matrix can't be inverted 3");
  }
  for (std::size_t i = 0; i < N; ++i) {
    xrow[i] = brow[i] / arow[N - 1];
    for (std::size_t jj = N - 1; jj > 0; --jj) {
      const auto j = jj - 1;
      const auto& ajrow = a[index[j]];
      auto& xji = x[j][i];
      xji = b[index[j]][i];
      for (std::size_t k = j + 1; k < N; ++k) xji -= ajrow[k] * x[k][i];
      if (ajrow[j] == 0) {
        throw std::runtime_error("Matrix can't be inverted 4");
      }
      xji /= ajrow[j];
    }
  }
  return x;
}

// Calculate an inverse of this matrix (version #2)
template <typename T>
XQSMatrix<T> XQSMatrix<T>::inverse_v2() const {
  if (m_nrows != m_ncols) {
    throw std::logic_error("Can't invert non-square matrix");
  }

  const std::size_t N = m_nrows;
  XQSMatrix rm(*this);
  XQSMatrix im = identity(N);
  const T zero = 0;
  T d;

  for (std::size_t i = 0; i < N - 1; i++) {
    auto& ri = rm.m_mat[i];
    d = ri[i];
    if (d == zero) {
      throw std::logic_error("Matrix can't be inverted 1");
    }
    auto& ii = im.m_mat[i];
    for (std::size_t col = 0; col < N; col++) {
      ri[col] /= d;
      ii[col] /= d;
    }
    for (std::size_t row = i + 1; row < N; row++) {
      auto& rr = rm.m_mat[row];
      auto& ir = im.m_mat[row];
      d = rr[i];
      for (std::size_t col = 0; col < N; col++) {
        rr[col] -= ri[col] * d;
        ir[col] -= ii[col] * d;
      }
    }
  }

  for (std::size_t i = N - 1; i > 0; i--) {
    auto& ri = rm.m_mat[i];
    d = ri[i];
    if (d == zero) {
      throw std::logic_error("Matrix can't be inverted 2");
    }
    auto& ii = im.m_mat[i];
    for (size_t col = 0; col < N; col++) {
      ri[col] /= d;
      ii[col] /= d;
    }
    for (size_t row = 0; row < i; row++) {
      auto& rr = rm.m_mat[row];
      auto& ir = im.m_mat[row];
      d = rr[i];
      for (size_t col = 0; col < N; col++) {
        rr[col] -= ri[col] * d;
        ir[col] -= ii[col] * d;
      }
    }
  }

  return im;
}

// Helper function for gaussian reduction
// used to find inverse matrix.
template <typename T>
std::vector<size_t> XQSMatrix<T>::gaussian_reduction() {
  const std::size_t N = m_nrows;
  std::vector<double> c(N);

  // Initialize index
  std::vector<size_t> index(N);
  for (std::size_t i = 0; i < N; ++i) index[i] = i;

  // Find the rescaling factors, one from each row
  for (std::size_t i = 0; i < N; ++i) {
    const auto& row = m_mat[i];
    double c1 = 0;
    for (std::size_t j = 0; j < N; ++j) {
      auto c0 = std::abs(row[j]);
      if (c0 > c1) c1 = c0;
    }
    c[i] = c1;
  }

  // Search the pivoting element from each column
  std::size_t k = 0;
  for (std::size_t j = 0; j < N - 1; ++j) {
    T pi1 = 0;
    for (std::size_t i = j; i < N; ++i) {
      T pi0 = std::abs(m_mat[index[i]][j]);
      if (c[index[i]] == 0) {
        throw std::runtime_error("Matrix can't be inverted 5");
      }
      pi0 /= c[index[i]];
      if (pi0 > pi1) {
        pi1 = pi0;
        k = i;
      }
    }

    // Interchange rows according to the pivoting order
    std::swap(index[k], index[j]);
    auto& row0 = m_mat[index[j]];
    const auto& v = row0[j];
    if (v == 0) {
      throw std::runtime_error("Matrix can't be inverted 6");
    }

    for (std::size_t i = j + 1; i < N; ++i) {
      auto& row = m_mat[index[i]];
      auto& v2 = row[j];
      auto pj = v2 / v;

      // Record pivoting ratios below the diagonal
      v2 = pj;

      // Modify other elements accordingly
      for (size_t l = j + 1; l < N; ++l) row[l] -= pj * row0[l];
    }
  }
  return index;
}

// Matrix/scalar addition
template <typename T>
XQSMatrix<T> XQSMatrix<T>::operator+(const T& rhs) const {
  XQSMatrix result(m_nrows, m_ncols);
  for (std::size_t i = 0; i < m_nrows; i++) {
    auto& row = m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      result.m_mat[i][j] = row[j] + rhs;
    }
  }
  return result;
}

template <typename T>
XQSMatrix<T>& XQSMatrix<T>::operator+=(const T& rhs) {
  for (std::size_t i = 0; i < m_nrows; i++) {
    auto& row = m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      row[j] += rhs;
    }
  }
  return *this;
}

// Matrix/scalar subtraction
template <typename T>
XQSMatrix<T> XQSMatrix<T>::operator-(const T& rhs) const {
  XQSMatrix result(m_nrows, m_ncols);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    auto& rrow = result.m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      rrow[j] = row[j] - rhs;
    }
  }
  return result;
}

template <typename T>
XQSMatrix<T>& XQSMatrix<T>::operator-=(const T& rhs) {
  for (std::size_t i = 0; i < m_nrows; i++) {
    auto& row = m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      row[j] -= rhs;
    }
  }
  return *this;
}

// Matrix/scalar multiplication
template <typename T>
XQSMatrix<T> XQSMatrix<T>::operator*(const T& rhs) const {
  XQSMatrix result(m_nrows, m_ncols);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    auto& rrow = result.m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      rrow[j] = row[j] * rhs;
    }
  }
  return result;
}

template <typename T>
XQSMatrix<T>& XQSMatrix<T>::operator*=(const T& rhs) {
  for (std::size_t i = 0; i < m_nrows; i++) {
    auto& row = m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      row[j] *= rhs;
    }
  }
  return *this;
}

// Matrix/scalar division
template <typename T>
XQSMatrix<T> XQSMatrix<T>::operator/(const T& rhs) const {
  XQSMatrix result(m_nrows, m_ncols);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    auto& rrow = result.m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      rrow[j] = row[j] / rhs;
    }
  }
  return result;
}

template <typename T>
XQSMatrix<T>& XQSMatrix<T>::operator/=(const T& rhs) {
  for (std::size_t i = 0; i < m_nrows; i++) {
    auto& row = m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      row[j] /= rhs;
    }
  }
  return *this;
}

// Obtain a vector of the diagonal elements
template <typename T>
std::vector<T> XQSMatrix<T>::diag_vec() const {
  std::vector<T> result(m_nrows);
  for (std::size_t i = 0; i < m_nrows; i++) {
    result[i] = m_mat[i][i];
  }
  return result;
}

// Multiply a matrix with a vector represeting single row
template <typename T>
XQSMatrix<T> XQSMatrix<T>::mul_by_row(const std::vector<T>& row_data) const {
  // Validate parameters
  if (row_data.empty()) {
    throw std::logic_error("Empty column data");
  }
  if (m_ncols != 1) {
    throw std::logic_error(
        "Matrix dimensions mismatch for product with vector row");
  }

  // Compute product
  const auto ncols = row_data.size();
  XQSMatrix<T> result(m_nrows, ncols, 0.0);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    for (std::size_t j = 0; j < ncols; j++) {
      result.m_mat[i][j] = row[j] * row_data[j];
    }
  }
  return result;
}

// Multiply a matrix with a vector representing single column
template <typename T>
std::vector<T> XQSMatrix<T>::mul_by_column(
    const std::vector<T>& column_data) const {
  // Validate parameters
  if (m_ncols != column_data.size()) {
    throw std::invalid_argument(
        "Input vector size mismatch for product with vector column");
  }

  // Compute product
  std::vector<T> result(m_nrows, 0.0);
  for (std::size_t i = 0; i < m_nrows; i++) {
    const auto& row = m_mat[i];
    for (std::size_t j = 0; j < m_ncols; j++) {
      result[i] += row[j] * column_data[j];
    }
  }
  return result;
}

// Scalar product of the row with a given vector
template <typename T>
T XQSMatrix<T>::row_scalar_product(std::size_t row_index,
                                   const std::vector<T>& v) const {
  validate_row_index(row_index);
  if (v.size() != m_ncols) {
    throw std::invalid_argument(
        "Input vector size mismatch for row scalar product");
  }
  T result = 0;
  const auto& row = m_mat[row_index];
  for (std::size_t i = 0; i < m_ncols; ++i) {
    result += row[i] * v[i];
  }
  return result;
}

// Scalar product of the column with a given vector
template <typename T>
T XQSMatrix<T>::column_scalar_product(std::size_t col_index,
                                      const std::vector<T>& v) const {
  validate_column_index(col_index);
  if (v.size() != m_nrows) {
    throw std::invalid_argument(
        "Input vector size mismatch for column scalar product");
  }
  T result = 0;
  for (std::size_t i = 0; i < m_ncols; ++i) {
    result += m_mat[i][col_index] * v[i];
  }
  return result;
}

// Add "count" columns at postion "pos" with inital value "v"
template <typename T>
void XQSMatrix<T>::add_columns(std::size_t pos, std::size_t count, const T& v) {
  validate_column_index(pos);
  for (auto& row : m_mat) {
    row.insert(row.begin() + pos, count, v);
  }
  m_ncols += count;
}

// Remove "count" columns at postion "pos"
template <typename T>
void XQSMatrix<T>::remove_columns(std::size_t pos, std::size_t count) {
  validate_column_index(pos);
  if (count > m_ncols - pos) {
    throw std::out_of_range("Removal count is out of range");
  }
  if (m_ncols == 1) {
    throw std::logic_error(
        "Can't remove column from matrix with single column");
  }
  for (auto& row : m_mat) {
    auto it = row.begin() + pos;
    row.erase(it, it + count);
  }
  m_ncols -= count;
}

// Set the number of m_nrows in the matrix
template <typename T>
void XQSMatrix<T>::set_row_count(std::size_t new_rows) {
  m_mat.resize(new_rows);
  m_nrows = new_rows;
}

// Get the number  of columns in the matrix
template <typename T>
void XQSMatrix<T>::set_col_count(std::size_t new_cols) {
  for (std::size_t i = 0; i < m_mat.size(); ++i) m_mat[i].resize(new_cols);
  m_ncols = new_cols;
}

template <typename T>
XQSMatrix<T> XQSMatrix<T>::row(std::size_t index) const {
  validate_row_index(index);
  XQSMatrix<T> result(1, m_ncols);
  result.m_mat[0] = m_mat[index];
  return result;
}

template <typename T>
std::vector<T> XQSMatrix<T>::row_as_vector(std::size_t index) const {
  validate_row_index(index);
  return m_mat[index];
}

template <typename T>
XQSMatrix<T> XQSMatrix<T>::column(std::size_t index) const {
  validate_column_index(index);
  XQSMatrix<T> result(m_nrows, 1);
  for (std::size_t i = 0; i < m_nrows; ++i) {
    result.m_mat[i][0] = m_mat[i][index];
  }
  return result;
}

template <typename T>
std::vector<T> XQSMatrix<T>::column_as_vector(std::size_t index) const {
  validate_column_index(index);
  std::vector<T> result(m_nrows);
  for (std::size_t i = 0; i < m_nrows; ++i) {
    result[i] = m_mat[i][index];
  }
  return result;
}

template <typename T>
XQSMatrix<T> XQSMatrix<T>::window(std::size_t row, std::size_t col,
                                  std::size_t nrows, std::size_t ncols) const {
  // Validate input parameters
  if (row >= m_nrows) {
    throw std::out_of_range("Row number is out of range");
  }
  if (col >= m_ncols) {
    throw std::out_of_range("Column number is out of range");
  }
  if (nrows == 0 || nrows > m_nrows - row) {
    throw std::out_of_range("Number of window rows is out of range");
  }
  if (ncols == 0 || ncols > m_ncols - col) {
    throw std::out_of_range("Number of window columns is out of range");
  }

  // Build new matrix
  XQSMatrix result(nrows, ncols);
  for (std::size_t i = 0; i < nrows; ++i) {
    auto& rrow = result.m_mat[i];
    for (std::size_t j = 0; j < ncols; ++j) {
      rrow[j] = m_mat[row + i][col + j];
    }
  }
  return result;
}

// Read from CSV file
template <typename T>
template <class Converter>
XQSMatrix<T> XQSMatrix<T>::readCsv(const std::string& path, char lineEnding,
                                   const std::string& fieldDelimiters,
                                   const Converter& converter,
                                   std::size_t numberOfHeaderLines) {
  XQSMatrix result(0, 0);

  // Open input file
  std::ifstream in(path.c_str());
  if (!in.is_open()) {
    throw std::runtime_error("Can't open input file");
  }

  // Skip header lines
  std::string line;
  size_t i = numberOfHeaderLines;
  while (i > 0 && std::getline(in, line, lineEnding)) {
    --i;
  }
  if (i > 0) {
    throw std::runtime_error("Missing some header m_nrows");
  }

  // Parse data lines
  size_t numberOfDataLines = 0;
  while (std::getline(in, line, lineEnding)) {
    ++numberOfDataLines;
    auto row = tokenizeAndParse(line, converter, fieldDelimiters);
    if (row.empty()) {
      throw std::runtime_error("There is empty data line");
    }
    if (result.m_ncols != row.size()) {
      if (result.m_ncols < row.size()) {
        result.set_col_count(row.size());
      } else {
        row.resize(result.m_ncols);
      }
    }
    result.m_mat.push_back(std::move(row));
    ++result.m_nrows;
  }

  // Ensure that at least one row have been successfully read
  if (numberOfDataLines == 0) {
    throw std::runtime_error("There is no data");
  }

  return result;
}

// Tokenize string and parse tokens as matrix cell values.
// This code is based on the public domain code taken from here:
// https://stackoverflow.com/a/1493195/1540501
template <typename T>
template <class Converter>
std::vector<T> XQSMatrix<T>::tokenizeAndParse(const std::string& str,
                                              const Converter& converter,
                                              const std::string& delimiters,
                                              bool trimEmpty) {
  std::vector<T> result;
  std::string::size_type pos, lastPos = 0, length = str.length();
  while (lastPos < length + 1) {
    pos = str.find_first_of(delimiters, lastPos);
    if (pos == std::string::npos) {
      pos = length;
    }
    if (pos != lastPos || !trimEmpty) {
      result.push_back(converter(str.substr(lastPos, pos - lastPos)));
    }
    lastPos = pos + 1;
  }
  return result;
}

// Check that matrix has equal dimensions
template <typename T>
inline void XQSMatrix<T>::check_equal_dimensions(
    const XQSMatrix<T>& other) const {
  if (m_nrows != other.m_nrows && m_ncols != other.m_ncols) {
    std::ostringstream err;
    err << "Dimensions of the other matrix differ (this vs other (rows*cols): "
        << m_nrows << "*" << m_ncols << " vs " << other.m_nrows << "*"
        << other.m_ncols << ")";
    throw std::invalid_argument(err.str());
  }
}

template <typename T>
inline void XQSMatrix<T>::check_suitable_for_product(
    const XQSMatrix<T>& other) const {
  if (m_ncols != other.m_nrows) {
    std::ostringstream err;
    err << "Dimensions of the other matrix are not suitable for the product "
           "this*other (this vs other (rows*cols): "
        << m_nrows << "*" << m_ncols << " vs " << other.m_nrows << "*"
        << other.m_ncols << ")";
    throw std::invalid_argument(err.str());
  }
}

template <typename T>
void XQSMatrix<T>::validate_row_index(std::size_t index) const {
  if (index >= m_nrows) {
    throw std::out_of_range("Row index is out of range");
  }
}

template <typename T>
void XQSMatrix<T>::validate_column_index(std::size_t index) const {
  if (index >= m_ncols) {
    throw std::out_of_range("Column index is out of range");
  }
}

template <class T, class CharT = char, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os, const XQSMatrix<T>& matrix) {
  typename std::basic_ostream<CharT, Traits>::sentry sentry(os);
  const auto nrows = matrix.row_count();
  const auto ncols = matrix.col_count();
  const auto& v = matrix.inner_vector();
  for (std::size_t i = 0; i < nrows; ++i) {
    const auto& row = v[i];
    for (std::size_t j = 0; j < ncols; ++j) {
      if (j > 0) {
        os << '\t';
      }
      os << row[j];
    }
    os << '\n';
  }
  return os;
}

#endif