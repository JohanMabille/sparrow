from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build.cppstd import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy
from conan.tools.microsoft import is_msvc
from conan.tools.scm import Version
import os

required_conan_version = ">=2.0"

class SparrowRecipe(ConanFile):
    name = "sparrow"
    description = "C++20 idiomatic APIs for the Apache Arrow Columnar Format"
    license = "Apache-2.0"
    author = "Man Group"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/man-group/sparrow"
    topics = ("arrow", "apache arrow", "columnar format", "dataframe")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps"
    exports_sources = "include/*", "LICENSE", "src/*", "cmake/*", "docs/*", "CMakeLists.txt", "sparrowConfig.cmake.in"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "use_date_polyfill": [True, False],
        "build_tests": [True, False],
        "build_benchmarks": [True, False],
        "generate_documentation": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "use_date_polyfill": False,
        "build_tests": False,
        "build_benchmarks": False,
        "generate_documentation": False,
    }

    def requirements(self):
        if self.options.get_safe("use_date_polyfill"):
            self.requires("date/3.0.3")
        if self.options.get_safe("build_tests"):
            self.test_requires("doctest/2.4.11")
            self.test_requires("catch2/3.7.0")
            self.test_requires("nlohmann_json/3.12.0")
        if self.options.get_safe("build_benchmarks"):
            self.test_requires("benchmark/1.9.4")

    def build_requirements(self):
        if self.options.get_safe("generate_documentation"):
            self.tool_requires("doxygen/1.9.4", options={"enable_app": "True"})

    @property
    def _min_cppstd(self):
        return 20

    @property
    def _compilers_minimum_version(self):
        return {
            "apple-clang": "16",
            "clang": "18",
            "gcc": "12",
            "msvc": "194"
        }

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)

        minimum_version = self._compilers_minimum_version.get(
            str(self.settings.compiler), False)
        if minimum_version and Version(self.settings.compiler.version) < minimum_version:
            raise ConanInvalidConfiguration(
                f"{self.ref} requires C++{self._min_cppstd}, which your compiler does not support."
            )

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self, src_folder=".")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["USE_DATE_POLYFILL"] = self.options.get_safe(
            "use_date_polyfill", False)
        tc.variables["BUILD_TESTS"] = self.options.get_safe("build_tests", False)
        tc.variables["BUILD_BENCHMARKS"] = self.options.get_safe(
            "build_benchmarks", False
        )
        if is_msvc(self):
            tc.variables["USE_LARGE_INT_PLACEHOLDERS"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        
    def package(self):
        copy(self, "LICENSE",
             dst=os.path.join(self.package_folder, "licenses"),
             src=self.source_folder)
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["sparrow"]
        self.cpp_info.set_property("cmake_file_name", "sparrow")
        self.cpp_info.set_property("cmake_target_name", "sparrow::sparrow")
