library XSLTProcessorTest;
import '../../pkg/unittest/lib/unittest.dart';
import '../../pkg/unittest/lib/html_individual_config.dart';
import 'dart:html';

main() {

  useHtmlIndividualConfiguration();

  group('supported', () {
    test('supported', () {
      expect(XsltProcessor.supported, true);
    });
  });

  group('functional', () {
    var isXsltProcessor =
      predicate((x) => x is XsltProcessor, 'is an XsltProcessor');

    var expectation = XsltProcessor.supported ? returnsNormally : throws;

    test('constructorTest', () {
      expect(() {
        var processor = new XsltProcessor();
        expect(processor, isNotNull);
        expect(processor, isXsltProcessor);
      }, expectation);
    });
  });

}
