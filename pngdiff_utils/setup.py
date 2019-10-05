import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="pngdiff_utils",
    version="0.0.1",
    author="Bocchio",
    author_email="guido@bocch.io",
    description="Utilities to work with pngdiff images",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/Bocchio/pngdiff",
    packages=setuptools.find_packages(),
	#entry_points={
	#	'console_scripts': [
	#		'png_diff=png_diff:main',
	#	],
	#}
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: POSIX :: Linux",
    ],
    python_requires='>=3.6',
)
