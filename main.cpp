

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <gdal.h>
#include <gdal_priv.h>
#include <gdalwarper.h>
#include <ogrsf_frmts.h>

#include <cpl_conv.h> // for CPLMalloc()

#include <iostream>
#include <algorithm>
#include <vector>


#include "shader.h"

using namespace std;

struct vert_t {

    float x;
    float y;
    float z;
    glm::vec4 col;

};



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
int processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);


float x_rot = 0;
float y_rot = 0;
float z_rot = 0;
float zoom = 0;
float tx, ty, tz = 0;
float h = 0;
float p = 0;

int main()
{
    // settings
    const unsigned int SCR_WIDTH = 1200;
    const unsigned int SCR_HEIGHT = 1200;

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "BenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    GLenum glew_error = glewInit();
    if (glew_error != GLEW_OK)
    {
        std::cout << "glewInit() error" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); //Enable blending.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);




    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("../vec_shader.vs", "../frag_shader.fs"); // you can name your shader files however you like
    Shader vec2("../vec2.vs", "../frag2.fs");
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------


    GDALDataset *poDataset;
    GDALDataset *refDataset;
    GDALRasterBand *poBand;
    //const char *fname = "../data/true.bil";
    //const char *ref = "../data/true.bil";
    const char *fname = "/home/ben/Desktop/pigeon2.tif";

    vert_t *vertices;

    GDALAllRegister();

    poDataset = (GDALDataset *) GDALOpen(fname, GA_ReadOnly);

    if (poDataset == nullptr) {
        std::cout << "File: " << fname << " not found.\n Exiting.\n";
        return 1;
    }


    int width = GDALGetRasterXSize(poDataset);
    int height = GDALGetRasterYSize(poDataset);
    int startx = 0;//800;
    int starty = 0;//800;

    double adfGeoTransform[6]; // Origin: 0,3 | Pix Size: 1,5
    poDataset->GetGeoTransform(adfGeoTransform);
    float orig_x = 0;//adfGeoTransform[0];
    float orig_y = 0;//adfGeoTransform[3];
    float res_x = adfGeoTransform[1];
    float res_y = adfGeoTransform[5];

    std::cout << orig_x << " " << orig_y << " " << res_x << " " << res_y << "\n";

    poBand = poDataset->GetRasterBand(1);
    double adfMinMax[2];



    GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);

    printf("MAx: %f, %f\n", adfMinMax[0], adfMinMax[1]);

    float max_z = adfMinMax[1];
    float min_z = adfMinMax[0];
    float max_x = orig_x + (width * res_x);
    float min_x = orig_x;
    float max_y = orig_y;
    float min_y = orig_y - (height * res_x);

    printf("%f, %f, %f, %f, %f, %f\n", max_x, min_x, max_y, min_y, max_z, min_z);

    vertices = (vert_t *)CPLMalloc(sizeof(vert_t) * width * height);


    float *pafScanline;
    pafScanline = (float *) CPLMalloc(sizeof(float)*width*height);
    auto err = poBand->RasterIO(GF_Read,startx,starty,width,height,pafScanline,width,height,GDT_Float32,0,0);

    std::vector<unsigned int> indices;

    float dx = max_x - min_x;
    float dy = max_y - min_y;

    float bigger = std::max(width * res_x, height * res_x);
    bigger = std::max(bigger, max_z);



    printf("Bigger: %f, %f, %f, %f\n", width * res_x, height * res_x, max_z, bigger);

    int vcount = 0;
    float colz = 0;
    //float zup, zdown, zleft, zright;
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {

            vert_t v;

            if((pafScanline[(i*width)+j]) < 0)
            {
                v.z = 0;
            }
            else{
                v.z = ((pafScanline[(i*width)+j]) / res_x) - min_z;
            }


            colz = ((pafScanline[(i*width)+j] - min_z) / (max_z - min_z));

            if(i == 0)
            {
                v.col = {1.0f, 0.0f, 0.0f,0.2f};

            }
            else if(j == 0)
            {
                v.col = {0.0f, 1.0f, 0.0f,0.2f};

            }
            else{
                v.col = {colz/2, colz,1.0f,0.2f};

            }


            //v.x = (((j * res_x) + orig_x) - min_x) / (bigger); // times x width + x orig
            //v.y = ((orig_y - (i * res_x)) - min_y) / (bigger) ; // this one will need checking

            v.x = (((j * res_x))); // times x width + x orig
            v.y = (((i * res_x))); // this one will need checking

            if((i < (height - 1)) && (j < (width - 1))) // Dont run off edge
            {
                indices.push_back((i * width) + j); // UL 0
                indices.push_back((i * width) + j + width + 1); // LR 3
                indices.push_back((i * width) + j + width); // LL 2
                indices.push_back((i * width) + j); // UL 0
                indices.push_back((i * width) + j + 1); // UR 1
                indices.push_back((i * width) + j + width + 1); // LR 3
            }

            vertices[vcount] = v;
            vcount++;

        }
    }

    float line_verts[] = {
            -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.5f,  0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f,  0.5f, 0.0f, 0.0f, 1.0f
    };

    unsigned int VBO, VAO, VBE, VBO1, VAO1;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &VBE);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vert_t) * width * height, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBE);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // position attribute
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenVertexArrays(1, &VAO1);
    glGenBuffers(1, &VBO1);

    glBindVertexArray(VAO1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO1);

    glBufferData(GL_ARRAY_BUFFER, sizeof(line_verts), line_verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    // glBindVertexArray(0);

    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
    unsigned int viewLoc = glGetUniformLocation(ourShader.ID, "view");
    unsigned int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");

    int vertexColorLocation = glGetUniformLocation(ourShader.ID, "color");

    unsigned int viewLoc2 = glGetUniformLocation(vec2.ID, "view");
    unsigned int projectionLoc2 = glGetUniformLocation(vec2.ID, "projection");


    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);




    glLineWidth(1.0);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render the triangle

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3((1/bigger) + zoom, (1/bigger)+zoom, (1/bigger)+ zoom + h));
        model = glm::translate(model, glm::vec3(-(width*res_x/2), -(height*res_x/2), 0.0f));

        view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(tx, ty, -2.0 + tz));
        view = glm::rotate(view, glm::radians(-60.0f + x_rot), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(y_rot), glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::rotate(view, glm::radians(20.0f + z_rot), glm::vec3(0.0f, 0.0f, 1.0f));

        projection = glm::perspective(glm::radians(45.0f + p), 1.0f, 0.01f, 100.0f);



        //glUniform4f(vertexColorLocation, 0.0f, 1.0f, 0.0f, 1.0f);

        vec2.use();
        glBindVertexArray(VAO1);
        glUniformMatrix4fv(viewLoc2, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc2, 1, GL_FALSE, glm::value_ptr(projection));


        glDrawArrays(GL_LINES, 0, 6);

        ourShader.use();
        glBindVertexArray(VAO);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));


        glDrawElements(
                GL_TRIANGLES,      // mode
                indices.size(),    // count
                GL_UNSIGNED_INT,   // type
                (void*)0           // element array buffer offset
        );




        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &VBO1);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    CPLFree(pafScanline);
    CPLFree(vertices);
    GDALClose(poDataset);
    GDALClose(refDataset);


    return 0;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {

    }
}



// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
int processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);

    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        x_rot += 1;

    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        x_rot -= 1;

    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        y_rot -= 1;

    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        y_rot += 1;

    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
    {
        z_rot += 1;

    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    {
        z_rot -= 1;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        zoom += 0.0000005;

    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        zoom -= 0.0000005;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        tx -= 0.01;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        tx += 0.01;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        ty += 0.01;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        ty -= 0.01;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        tz += 0.01;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
    {
        tz -= 0.01;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        h += 0.0000005;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        h -= 0.0000005;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
    {
        p -= 1.0;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        p += 1.0;
    }

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
    {
        tx = 0;
        ty = 0;
        tz = 0;
        zoom = 0;
        x_rot = 0;
        y_rot = 0;
        z_rot = 0;
        h = 0;
    }


    return 0;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


