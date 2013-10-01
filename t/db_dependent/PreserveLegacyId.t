#!/usr/bin/perl
#
# This Koha test module is a stub!
# Add more tests here!!!

use Modern::Perl;
use utf8;

use Test::More tests => 3;
use MARC::Record;

BEGIN {
        use_ok('C4::Record');
        use_ok('C4::Biblio');
}

### Preparing our testing data ###
my $bibFramework = ''; #Using the default bibliographic framework.
my $marcxml=qq(<?xml version="1.0" encoding="UTF-8"?>
<record format="MARC21" type="Bibliographic">
  <leader>00000cim a22000004a 4500</leader>
  <controlfield tag="001">1001</controlfield>
  <controlfield tag="005">2013-06-03 07:04:07+02</controlfield>
  <controlfield tag="007">ss||||j|||||||</controlfield>
  <controlfield tag="008">       uuuu    xxk|||||||||||||||||eng|c</controlfield>
  <datafield tag="020" ind1=" " ind2=" ">
    <subfield code="a">0-00-103147-3</subfield>
    <subfield code="c">14.46 EUR</subfield>
  </datafield>
  <datafield tag="041" ind1="0" ind2=" ">
    <subfield code="d">eng</subfield>
  </datafield>
  <datafield tag="084" ind1=" " ind2=" ">
    <subfield code="a">83.5</subfield>
    <subfield code="2">ykl</subfield>
  </datafield>
  <datafield tag="100" ind1="1" ind2=" ">
    <subfield code="a">SHAKESPEARE, WILLIAM.</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="4">
    <subfield code="a">THE TAMING OF THE SHREW /</subfield>
    <subfield code="c">WILLIAM SHAKESPEARE</subfield>
    <subfield code="h">[ÄÄNITE].</subfield>
  </datafield>
  <datafield tag="260" ind1=" " ind2=" ">
    <subfield code="a">LONDON :</subfield>
    <subfield code="b">COLLINS.</subfield>
  </datafield>
  <datafield tag="300" ind1=" " ind2=" ">
    <subfield code="a">2 ÄÄNIKASETTIA.</subfield>
  </datafield>
  <datafield tag="852" ind1=" " ind2=" ">
    <subfield code="a">FI-Jm</subfield>
    <subfield code="h">83.5</subfield>
  </datafield>
  <datafield tag="852" ind1=" " ind2=" ">
    <subfield code="a">FI-Konti</subfield>
    <subfield code="h">83.5</subfield>
  </datafield>
</record>
);

my $record=marcxml2marc($marcxml);

my ( $biblionumberTagid, $biblionumberSubfieldid ) =
            GetMarcFromKohaField( "biblio.biblionumber", $bibFramework );
my ( $biblioitemnumberTagid, $biblioitemnumberSubfieldid ) =
            GetMarcFromKohaField( "biblioitems.biblioitemnumber", $bibFramework );

# Making sure the Koha to MARC mappings stand correct
is ($biblionumberTagid, $biblioitemnumberTagid, 'Testing "Koha to MARC mapping" biblionumber and biblioitemnumber source MARC field sameness');
if ($biblionumberTagid ne $biblioitemnumberTagid) {
	warn "Koha to MARC mapping values biblio.biblionumber and biblioitems.biblioitemnumber must share the same MARC field!";
}

#Moving the 001 to the field configured for legacy id
my $legacyIdField = $record->field( '001' );
my $legacyId = $legacyIdField->data();
my $biblionumberField = MARC::Field->new( $biblionumberTagid, '', '',
   $biblionumberSubfieldid => $legacyId,
   $biblioitemnumberSubfieldid => $legacyId,
);
$record->append_fields($biblionumberField);

#Convenience method to easily change the legacy id.
# Is used to test out database id boundary values.
sub changeLegacyId {
    my ($record, $newId) = @_;

    # Because the author of Marc::Record decided it would be nice to replace all subfields with new ones whenever they are updated,
    #  old references are lost and need to be found again.
    my $legacyIdField = $record->field( '001' );
    my $biblionumberField = $record->field( $biblionumberTagid );

    $legacyIdField->update( $newId );
    $biblionumberField->update( $biblionumberSubfieldid => $newId,
                                $biblioitemnumberSubfieldid => $newId);
    $legacyId = $newId;


}

##############################
### Running the main test! ###
##############################

## INSERT the record to the DB, SELECT the one we got,
## then find out if the biblionumber and biblioitemnumbers are the original ones.

my $dbh = C4::Context->dbh;

$dbh->{AutoCommit} = 0; #We don't want to save anything in DB as this is just a test run!
$dbh->{RaiseError} = 1;

##Testing just below the critical threshold in _koha_add_item() primary key validators.
##  Test should run OK. Also this biblionumber hardly is reserved :)
changeLegacyId($record, 2147483639);

my ( $newBiblionumber, $newBiblioitemnumber ) = AddBiblio( $record, $bibFramework, { defer_marc_save => 1 } );
my $selectedBiblio = GetBiblio($legacyId);

is ($legacyId, $newBiblionumber, 'Biblionumber returned by AddBiblio matches');
is ($legacyId, $selectedBiblio->{biblionumber}, 'Biblionumber returned by GetBiblio matches the legacy biblionumber');


##Testing primary key exhaustion situation. Validator should prevent biblionumber or biblioitemnumber INSERTion
##  if the value is over the safety threshold 2147483639 == LONG_MAX
##  So both tests should fail.
changeLegacyId($record, 2147483645);

( $newBiblionumber, $newBiblioitemnumber ) = AddBiblio( $record, $bibFramework, { defer_marc_save => 1 } );
$selectedBiblio = GetBiblio($legacyId);

isnt ($legacyId, $newBiblionumber, 'Testing primary key exhaustion prevention.');
isnt ($legacyId, $selectedBiblio->{biblionumber}, 'Primary key exhausted biblio matching');

##Testing bad primary key situation. Validator should prevent biblionumber or biblioitemnumber INSERTion
##  if the value is not a pure integer/long.
##  So both tests should fail.
changeLegacyId($record, '12134FAN');

( $newBiblionumber, $newBiblioitemnumber ) = AddBiblio( $record, $bibFramework, { defer_marc_save => 1 } );
$selectedBiblio = GetBiblio($legacyId);

isnt ($legacyId, $newBiblionumber, 'Testing primary key not-an-integer prevention.');
isnt ($legacyId, $selectedBiblio->{biblionumber}, 'Primary key not-an-integer biblio matching');


$dbh->rollback();
$dbh->disconnect();
