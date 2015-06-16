=====
USAGE
=====

* Create a bash script which calls this function however you like. Likely that means waiting until a file is available in the directory then calling "./ObjectClassifier f <file>" to process it.
* The application outputs a file called "detected.csv". It will append to this file if it exists, or create it if it doesn't.
* Once processed, "detected.csv" should be deleted so that there are no errors regarding which frame the detection references.

=====
OUTPUT FORMAT
=====

* The file "detected.csv" is comma-separated with no headings. The columns are:
	1. Letter
	2. Orientation (degrees relative to "up", I think clockwise?)
	3. Shape (string)
	4. Letter color (BGR I think)
	5. Shape color (BGR I think)
	6. Location of centroid (?) within frame (x, y) (I think the coordinates are WRT the top-left corner)

=====
SHAPE DETECTION
=====

* Perhaps the most under-developed part of this application is the shape classification. If you can do it better, please submit an alternative or an addition to the following functoin. Should be pretty easy to add star, etc. support:

ShapeType::ShapeType GeometryClassifier::Classify(vector<Point> vertices)
{
    //Invalid:
    if (vertices.size() < 3)
        return ShapeType::Invalid;

    //Triangle:
    if (vertices.size() == 3)
        return ShapeType::Triangle;

    //Must be placed near the beginning because it overwrites shapes like hexagon (which may be a star):
    //Star - do the vertex angles alternate in pairs?
    if (vertices.size() >= 6 && vertices.size() % 2 == 0) //even and 6 or more vertices
    {
        vector<float> angles;
        VertexAngles(vertices, angles);

        //Detect alternating sequence of vertex angles (high, low, high, low, ...):
        int alternating = 1;
        if (angles[1] - angles[0] > 0) //initially increasing
        {
            for (int i = 1; i < angles.size(); i+=2)
            {
                if (angles[i+1] - angles[i] > 0) //should be decreasing
                    alternating = 0;
                if (angles[i+2] - angles[i+1] < 0 ) //should be increasing
                    alternating = 0;
            }
        }
        else //initially decreasing
        {
            for (int i = 1; i < angles.size(); i+=2)
            {
                if (angles[i+1] - angles[i] < 0) //should be increasing
                    alternating = 0;
                if (angles[i+2] - angles[i+1] > 0 ) //should be decreasing
                    alternating = 0;
            }
        }

        if (alternating)
            return ShapeType::Star;
    }

    //Square/rhombus:
    if (vertices.size() == 4  & isContourConvex(vertices))
    {
        //Calculate vertex angles:
        vector<float> angles;
        VertexAngles(vertices, angles);

        //Square - are the angles approx. 90 degrees? Otherwise, rhombus
        int incorrectAngleDetected = 0;
        int delta = 10;
        for (int i = 0; i < 4; i++)
        {
            if (angles[i] < 90 - delta || angles[i] > 90 + delta)
                incorrectAngleDetected = 1;
        }

        if (!incorrectAngleDetected)
            return ShapeType::Square;

        //Rhombus - do the angles occur in equal opposite pairs?
        float d1 = angles[0] - angles[2];
        float d2 = angles[1] - angles[3];
        float threshold = 10;
        if (abs(d1) < threshold && abs(d2) < threshold)
            return ShapeType::Rhombus;

        //Otherwise, it's a trapezoid:
        return ShapeType::Trapezoid;
    }

    //Hexagon:
    if (vertices.size() == 6 & isContourConvex(vertices))
    {
        return ShapeType::Hexagon;
    }

    //Cross:
    if (vertices.size() == 12)
    {
        //Calculate vertex angles:
        vector<float> angles;
        VertexAngles(vertices, angles);

        //Are the angles approx. 90 degrees?
        int incorrectAngleDetected = 0;
        int delta = 10;
        for (int i = 0; i < 12; i++)
        {
            if (angles[i] < 90 - delta || angles[i] > 90 + delta)
                incorrectAngleDetected = 1;
        }

        if (!incorrectAngleDetected)
            return ShapeType::Cross;
    }

    //PLACE AT END!!! This is because this is the least "specific" of the classifications
    //Circle and semicircle - hard to classify but can look for a lot of vertices and convexity:
    if (vertices.size() > 8 & isContourConvex(vertices))
    {
        //Get bounding rectangle:
        Rect bound = boundingRect(vertices);

        //Look for a long line:
        int longLineFound = 0;
        float len;
        for (int i = 1; i < vertices.size(); i++)
        {
            len = sqrt(pow(vertices[i].x - vertices[i-1].x,2) + pow(vertices[i].y - vertices[i-1].y,2));
            //printf("%f, %f, %f\n", (float)len, (float)bound.height, (float)bound.width);
            if (len > bound.height/2 | len > bound.width/2)
                longLineFound = 1;
        }

        //Comparison between 0 and vertices.size()-1
        if (vertices.size() > 1)
        {
            len = sqrt(pow(vertices[0].x - vertices[vertices.size()-1].x,2) + pow(vertices[0].y - vertices[vertices.size()-1].y,2));
            //printf("%f, %f, %f\n", (float)len, (float)bound.height, (float)bound.width);
            if (len > bound.height/2 | len > bound.width/2)
                longLineFound = 1;
        }

        if (longLineFound)
            return ShapeType::Hemicircle;
        else
            return ShapeType::Circle;
    }

    //If not classified by this time, it's invalid:
    return ShapeType::Invalid;
}
