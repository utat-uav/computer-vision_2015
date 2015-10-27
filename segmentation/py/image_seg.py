import cv2
import numpy as np
import sys

class Segment:
	def __init__(self, MAX_CLUSTERS = None, MAX_NUM_KMEANS_ITER = None, MAX_KMEANS_EPSILON = None):
		self.MAX_CLUSTERS = MAX_CLUSTERS or 3
		self.MAX_NUM_KMEANS_ITER = MAX_NUM_KMEANS_ITER or 25
		self.MAX_KMEANS_EPSILON = MAX_KMEANS_EPSILON or 1.0
		
		self.KMEANS_TERMINATOR = (cv2.TERM_CRITERIA_MAX_ITER + cv2.TERM_CRITERIA_EPS, self.MAX_NUM_KMEANS_ITER, self.MAX_KMEANS_EPSILON)

		# For post processing
		# Numbers may vary depending on image resolution
		# TODO: fix that -- scale with resolution -- 
		self.ERODE_KERNEL = np.ones((3, 3),np.uint8)
		self.DILATE_KERNEL = np.ones((6, 6),np.uint8)
	
	def postProcessImg(self, image, tag):
		# Cleans up remaining noise
		cv2.erode(image, self.ERODE_KERNEL, image)
		cv2.dilate(image, self.DILATE_KERNEL, image)
		cv2.imwrite("processed"+tag+".jpg", image)
		outImage, contours, hierarchy = cv2.findContours(image, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)

		rows, cols = len(image), len(image[0])

		# Find largest contour length and area
		largestArea = 0
		largestLen = 0
		for contour in contours:
			area = cv2.contourArea(contour)
			if largestLen < len(contour):
				largestLen = len(contour)
			if largestArea < area:
				largestArea = area

		# Finds best scoring contour based on area (and possibly other factors)
		largestScore = 0
		currentContour = contours[0]
		currentContourIndex = 0
		for i, contour in enumerate(contours):
			area = cv2.contourArea(contour)
			newScore = area/largestArea #+ len(contour)/largestLen
			if largestScore < newScore and area < (rows*cols*0.9):
				largestScore = newScore
				currentContour = contour
				currentContourIndex = i
			
		# Approximates the contour
		approx = cv2.approxPolyDP(contours[currentContourIndex], 0.007*cv2.arcLength(contours[currentContourIndex], True), True)
		approxArray = [approx]

		# Outputs the image
		polyApproxImage = np.zeros((rows, cols, 3))
		cv2.drawContours(polyApproxImage, approxArray, 0, (255, 255, 255), -1) 
		cv2.imwrite("hull"+tag+".jpg", polyApproxImage)

	def segment(self, img_path):
		print("Converting to Lab...")
		img = cv2.imread(img_path)
		img = cv2.blur(img, (3, 3))
		rows, cols = len(img), len(img[0])
		lab_img = cv2.cvtColor(img, cv2.COLOR_RGB2HSV)
		cv2.imwrite("labim.jpg", lab_img)
		
		print("Rows:", rows, "Cols:", cols)
		#only transfer 2nd and 3rd (A and B) channels to `abim`
		lab_channels = cv2.split(lab_img)
		# Possible removal of channels
		#lab_channels = np.delete(lab_channels, 1, 0)
		abim = cv2.merge(lab_channels)
		
		print("Reshaping...")
		abim_flat = np.float32(np.reshape(abim, (rows*cols, 3))) #flatten to a rows*col length array with 2 channels
		
		print("Running KMeans")
		retval, labels_flat, centers = cv2.kmeans(abim_flat, self.MAX_CLUSTERS, None, self.KMEANS_TERMINATOR, self.MAX_NUM_KMEANS_ITER, cv2.KMEANS_RANDOM_CENTERS)
		
		print("Reshaping...")
		#transform from flat to original image dimensions by feeding in data throught he buffer row-by-row
		labels = np.reshape(labels_flat, (rows, cols)) #reshape the flat labels array to original dimensions
		
		print("Saving labels...")
		#{MAX_CLUSTERS}-color greyscale image
		cv2.imwrite("labels.jpg", labels*70)
		
		isolated_labels = []
		for i in range(self.MAX_CLUSTERS):
			#map equality function onto all elements in `labels` and push the result (a matrix the size of `labels` with only 0 or 1) onto `isolated_labels`. It's important to cast the Bool to np.int since `findContours` won't accept an array "of type=0"
			isolated_labels.append(np.vectorize(lambda x: np.int(x==i))(labels))
			cv2.imwrite("labels"+str(i)+".jpg", isolated_labels[i]*255)
			imgProcessed = cv2.imread("labels"+str(i)+".jpg", cv2.IMREAD_GRAYSCALE)
			self.postProcessImg(imgProcessed, str(i))
			#cv2.imwrite("labels"+str(i)+"PROCESSED.jpg", imgProcessed)
		
		##CONVEXHULL
		
		#first, generate a list of contours, each a list of tuples (x,y)
		#image, shape_contours, hierarchy = cv2.findContours(isolated_labels[2].astype(np.uint8), cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
		#Find the most complex shape: the contour with the most points
		#most_complex_shape = reduce(lambda x, y: x if len(x)>len(y) else y, shape_contours)
		#Run `convexHull`
		#outHull = cv2.convexHull(most_complex_shape)
		
		#create a blank, black image and draw the simplified shape onto it
		#hull_img = np.zeros((rows, cols, 3))
		#cv2.fillConvexPoly(hull_img, outHull, [255, 255, 255])
		
		#cv2.imwrite("hull.jpg", hull_img)

if __name__ == "__main__":
	Segment().segment(sys.argv[1])