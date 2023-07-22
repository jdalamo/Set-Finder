# Set-Finder
iOS app that finds Set® sets in real-time

### Description
This app can be used to detect sets in the popular card game Set®.  To use it, launch the app and then point the camera at a grid of Set® cards.  The app will highlight all sets it finds.  Sets are color-coded and if a card is a member of multiple sets it will be highlighted with each set's color.

### How it works
Frames are read from the back camera and analyzed with OpenCV as part of the following algorithm I wrote:

1. Threshold the image
   - Thresholding is the process of setting every pixel below a certain intensity threshold to 0 (black) and every pixel above that intensity threshold to 255 (white).
2. Find contours
   - A contour is any border between black and white pixels.
3. Filter out card contours from all contours
   - Card contours can be identified with the following criteria:
      - It has at least one "child contour."  A child contour is another contour that is contained within the card contour.
      - The area of the contour is greater than a certain area threshold, which is a percentage of the frame's size.
      - The contour's shape is a rectangle.
      - The aspect ratio of the contour is within a certain threshold.
4. Filter out shape contours from all contours
   - Shape contours can be identified with the following criteria:
      - The contour's parent contour is a card contour.
      - The area of the contour is greater than a certain threshold, which is a percentage of the minimum card contour area.
      - The calculated "approximate" contour of this contour is a rectangle.
5. Classify each shape contour while keeping a reference to its associated card
   - Each shape contour must be classified into a shape object, which has the following attributes:
      - Symbol
      - Color
      - Shading
   - The symbol is determined by comparing the actual contour with a 4-sided approximation of the contour.  If the similarity is below a certain threshold then the symbol is a diamond, because of the three possible symbols only diamonds can be accurately approximated with 4 sides.
   - The color is determined by analyzing the average color of the border of the shape.  This produces more accurate results than analyzing the average color of the whole shape since for "OPEN" and "STRIPED" shapes most of the inside is white.
   - The shading is determined by comparing the average color of a region just outside the shape with the average color of a region just inside the shape.  The reason for this is twofold:
      - First, this method does not consider the color of the border of the shape.  That is important for distinguishing between "OPEN" and "STRIPED" shapes.  When trying to differentiate between "OPEN" and "STRIPED", if you include the border color in the average color calculation the difference gets washed out, so to speak.
      - Second, this method is more robust than comparing the average color of a region just inside the shape with, for example, the average color of all cards (which would be more efficient since we could calculate that once per frame instead of once per shape).  It's more robust because it's very likely that different cards will be experiencing different levels of shadow/illumination.  By calculating this per-shape, this algorithm remains unimpacted by that possibility.
6. Verify cards
   - For each card all the classified shapes are analyzed to ensure that the algorithm didn't classify multiple shapes on the same card differently.
7. Iterate through all cards while searching for sets.

### Planned improvements
At this stage the app is a proof-of-concept.  It works well in favorable lighting conditions when used by someone with a steady hand but I have a few enhancements planned that will improve the performance:
- Add multithreading
   - Every step in the algorithm runs quickly enough to be used in a single-threaded implementation except for step 5.  The work required to classify the shapes accurately takes too long for my taste.  The app works, but the average frame processes in roughly ~200 milliseconds which means the live feed is quite laggy.  Ideally I would like this to run in real-time, which should be possible once I make the shape classification step multi-threaded.
- Add machine learning
   - A no-machine-learning approach can get you pretty far with this particular problem, but there is at least one place where I think it could help:
      - Picking the correct threshold value is fundamental to getting any kind of accuracy out of this app.  If it's too low, the frame will contain a lot of noise that could be mistaken for cards by the card filtering logic.  If it's too high, the cards themselves will be washed out and the card filtering logic will fail to detect them entirely.  The current implementation sets the threshold value by first blurring the image, then getting the maximum pixel intensity value, and then taking a percentage of that as the threshold value.  This works well enough for a proof-of-concept but I would like to try to make this more accurate.  On top of that, I think machine learning could improve the performance because the blurring logic I have in place is quite an expensive operation.  Because of that, I only apply the blur once every 6 frames.  So on top of an accuracy improvement, I believe machine learning could help with the performance as well.
