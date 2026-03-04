from setuptools import setup
import subprocess
import shutil
import sys
import os

NAME = "PowerFlowPython"


def build_project():
    build_dir = "build"
    python_dir = os.path.abspath("python")
    os.makedirs(python_dir, exist_ok=True)

    # Make sure we have pybind11 installed.
    try:
        import pybind11

        pybind_cmake_path = pybind11.get_cmake_dir()
    except ImportError:
        print("Error: pybind11 not found in environment.")
        return

    # Build our C++ code with the Python flag to generate the .pyd | .so file.
    subprocess.run(
        ["cmake", "-S", ".", "-B", build_dir, f"-Dpybind11_DIR={pybind_cmake_path}"],
        check=True,
    )
    subprocess.run(["cmake", "--build", build_dir, "--config", "Release"], check=True)

    # Find the .pyd | .so file.
    binary_filename = None

    # Check both the Release folder (Windows) and the direct python folder (Linux/Mac)
    search_locations = [
        os.path.join(build_dir, "python", "Release"),
        os.path.join(build_dir, "python"),
        build_dir,
    ]

    # Find the file. 
    for d in search_locations:
        if os.path.exists(d):
            for f in os.listdir(d):
                if f.startswith(NAME) and f.endswith((".pyd", ".so")):
                    binary_filename = f
                    shutil.copy(os.path.join(d, f), os.path.join(python_dir, f))
                    print(f"--- Successfully copied binary: {f} ---")
                    break
            if binary_filename:
                break

    # Create the type files (stubs)
    if binary_filename:

        # Create a temporary copy of system variables and prepend the 'python' folder 
        # to PYTHONPATH so the stub generator can 'import PowerFlowPython' from the local directory.
        env = os.environ.copy()
        env["PYTHONPATH"] = python_dir + os.pathsep + env.get("PYTHONPATH", "")

        try:
            # Run the command to generate the stubs based on the .pyd | .so file
            subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "pybind11_stubgen",
                    NAME,
                    "-o",
                    python_dir,
                ],
                env=env,
                check=True,
            )

            # Copies the pyi file to the '/python' directory.
            target_stub = os.path.join(python_dir, f"{NAME}.pyi")
            for root, _, files in os.walk(python_dir):
                if f"{NAME}.pyi" in files:
                    src = os.path.join(root, f"{NAME}.pyi")
                    if os.path.abspath(src) != os.path.abspath(target_stub):
                        shutil.copy(src, target_stub)
                        break
        except Exception as e:
            print(f"--- Stub generation failed: {e} ---")
    else:
        print("--- Error: Binary file not found. Skipping stub generation. ---")


if __name__ == "__main__":
    build_project()

    setup(
        name=NAME,
        version="1.0.0",

        # Tells setuptools that the root package code lives in the 'python' folder
        package_dir={"": "python"},

        # Identifies the specific Python module/extension to include
        py_modules=[NAME],

        # Make sure to bundle .pyi, .pyd and .so files together
        package_data={"": ["*.pyi", "*.pyd", "*.so"]},

        # Makes sure setuptools doesn't just ignore the package_data rules during installation
        include_package_data=True,

        # Required for C++
        zip_safe=False,
    )
