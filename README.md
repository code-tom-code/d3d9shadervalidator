# D3D9 Shader Validator

`IDirect3DShaderValidator9` is an undocumented Direct3D9 C/C++ interface that is exposed through the `Direct3DShaderValidatorCreate9` function, which is a DLL-exported named function in `d3d9.dll`. A header file (with many missing definitions) was created for this interface, which is available [here](d3d9_shadervalidator.h). Additionally, a working code sample illustrating how to use this undocumented interface is also included in this GitHub repo called "D3D9ShaderValidatorSample".

## Who Is This For?

Potential users of this interface might include:
  * People working with shader compiler tools for Shader Models 1, 2, and/or 3.
  * Shader authors looking to debug issues with their shaders
  * Documentarians seeking details of the enforced rules of Direct3D9 that may not be explicitly stated in the official MSDN documentation
  * Conspiracy theorists who are not sure what that one weird function exported in a very commonly-used DLL is for

## Getting Started (Developers)

Just download the header file from here and #include it in your code: [d3d9_shadervalidator.h](d3d9_shadervalidator.h)

If you are interested in checking out the sample code, you can clone or download this repo, and then build and run [the Solution file](D3D9ShaderValidatorSample.sln) with a recent version of Visual Studio (I used VS2017 here, but it's likely that older versions will work too just fine).

## Authors

* **Tom Lopes** - *Initial work* - [code-tom-code](https://github.com/code-tom-code)

## License

This project (including all files in this distribution) is licensed under the zLib/LibPNG License - see the [LICENSE.txt](LICENSE.txt) file for details
