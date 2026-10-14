// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <benchmark/benchmark.h>

module FileGuard;
import :AllOrNothing;

import std;

static constexpr std::string_view kDummyText
{
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit.Quisque vitae tempus sapien.Vivamus sit amet volutpat augue.Fusce orci tellus, congue et convallis at, hendrerit a justo.Aenean sit amet condimentum magna, eget interdum dolor.Aliquam id magna bibendum, finibus tortor sit amet, porta purus.Curabitur efficitur vehicula turpis eu dictum.Vestibulum vel molestie ipsum.Suspendisse sollicitudin tristique lorem mattis interdum.Maecenas et dolor interdum, rutrum tortor eu, tincidunt enim."
    "Phasellus commodo lorem nisi, ac accumsan sem elementum quis.Curabitur bibendum tincidunt massa, feugiat volutpat dolor feugiat pellentesque.Vestibulum odio est, interdum ac tincidunt eget, sagittis in urna.Cras gravida lectus et velit lacinia, ac volutpat libero suscipit.Etiam arcu enim, hendrerit id quam non, laoreet volutpat tellus.Phasellus maximus luctus eros eu laoreet.Sed vel nisi at neque maximus bibendum.Duis vehicula purus id urna finibus volutpat.Vivamus varius tristique dui sed rhoncus."
    "Morbi ut sem sem.Duis aliquam, orci sed rhoncus aliquet, metus velit sollicitudin neque, vel sodales tellus ipsum at justo.Ut dignissim eros sed orci dictum, molestie dictum erat finibus.Curabitur egestas turpis et leo vehicula, sagittis pharetra risus convallis.Nunc egestas elit sed diam auctor vestibulum.Cras aliquet, metus non fermentum hendrerit, risus ante volutpat purus, sed mattis dui velit in ligula.Aliquam quis quam sed tellus ornare porta vitae a sapien.Aliquam ullamcorper ante id massa dapibus, vitae eleifend lorem tempor.Donec ornare nisl eu neque pellentesque, nec volutpat mi scelerisque."
    "Vestibulum tempor, felis et accumsan porta, lectus ex posuere lectus, ac tincidunt erat nisl ac urna.Nam rutrum, neque in semper fermentum, lacus turpis molestie ligula, sit amet bibendum risus nisl nec odio.Proin vestibulum ex ut magna fringilla, a venenatis nulla vestibulum.Maecenas lorem justo, varius a suscipit eget, euismod sit amet turpis.Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas.Donec auctor nisi et lacus feugiat hendrerit.Nunc congue euismod consequat.Pellentesque tempus odio non est commodo, ac commodo magna vulputate.Fusce imperdiet eros a erat finibus, elementum malesuada mi commodo.Nam feugiat volutpat sagittis.Pellentesque at nisi ultricies, mattis ante ut, fringilla lacus.Cras odio nulla, tincidunt ac magna ut, ultrices aliquam tellus.Curabitur sem erat, molestie id congue a, pretium eget purus.Aenean id diam sed massa laoreet vestibulum."
    "Nam fermentum tincidunt nunc, nec dapibus ligula dignissim convallis.Suspendisse mi tortor, vulputate sed ante sit amet, bibendum facilisis velit.Suspendisse pellentesque tortor sit amet finibus mattis.Vivamus rutrum nisi velit, ut finibus ante placerat sit amet.Nam et nibh a nulla luctus vestibulum vel eu mauris.Nulla consequat justo at enim blandit semper.Vivamus et nulla dolor.Nullam efficitur in mauris id cursus.Integer convallis arcu orci, id ultrices orci facilisis a."
    "Quisque dictum sodales elit, at laoreet enim dictum sit amet.Suspendisse quis sapien rhoncus, aliquam orci ut, aliquet risus.Vivamus placerat lorem orci, sit amet consequat lorem porta id.Donec ac facilisis nulla, eu posuere ante.Aenean at felis est.Duis commodo egestas nibh at vehicula.Mauris augue risus, luctus et faucibus vulputate, iaculis eget nibh.Nulla luctus ex sit amet arcu ullamcorper maximus."
    "Donec vitae sapien eu eros rhoncus ultrices commodo nec justo.Lorem ipsum dolor sit amet, consectetur adipiscing elit.Vivamus placerat cursus pharetra.Vestibulum placerat mi quis lacus feugiat elementum.Aenean a congue felis, sit amet feugiat urna.Proin quis hendrerit nulla.Nunc tincidunt maximus tempor.Sed vulputate ligula in quam vestibulum, nec interdum orci hendrerit."
    "Fusce tincidunt convallis augue vitae sodales.Donec sit amet ipsum ut dui congue vehicula vitae at massa.Suspendisse consectetur laoreet dui vitae volutpat.Maecenas feugiat posuere eleifend.Vivamus elementum, orci a congue consequat, quam ipsum porttitor nisi, vitae tempus sem nisl at elit.Nullam euismod elementum lectus, fermentum cursus ante tincidunt id.Quisque lectus tortor, suscipit at est vel, mollis blandit orci.Quisque quis nulla ac purus consequat efficitur ac et massa.Ut congue diam ut libero congue porta at sit amet sem.Vestibulum vitae justo sollicitudin, hendrerit mauris id, ultrices tortor.Suspendisse sed aliquam urna.Mauris viverra tellus quis nisi aliquet, id sagittis eros ultricies.Vivamus molestie tristique dui tincidunt malesuada.In tristique velit sit amet odio bibendum consequat."
    "Etiam ligula est, tincidunt sed mi vel, pretium condimentum leo.Nam semper ex nec leo vehicula sodales.Integer varius, augue in interdum accumsan, est erat fringilla nisi, et viverra tortor neque eu dolor.Duis fermentum ex nec lacus fringilla, quis euismod elit posuere.Aenean sit amet turpis rhoncus erat egestas scelerisque tempor quis enim.Aliquam erat volutpat.Vivamus blandit odio eget magna sodales congue.Donec neque tellus, blandit quis dui sit amet, tincidunt accumsan diam.Sed scelerisque, lorem ac tincidunt ullamcorper, justo magna dapibus turpis, in consectetur eros mauris vitae massa.Maecenas eget ex in risus posuere dictum id at odio.Donec turpis ex, porta vel risus non, aliquet consequat arcu.Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Ut placerat dapibus mauris vehicula ultrices.Donec velit purus, tristique a placerat vitae, posuere sit amet elit.Mauris vulputate eros in felis ornare fringilla.Sed tempus diam sapien, et tempus justo tincidunt ac."
    "Donec vulputate gravida consectetur.Aenean in egestas neque.Nulla mauris sem, dignissim sit amet dui ac, condimentum aliquam urna.Integer porttitor rhoncus gravida.Sed turpis nisl, suscipit eget ex a, lacinia scelerisque lacus.Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Etiam gravida porta elementum.Phasellus in tincidunt sem, at rhoncus dolor.Morbi eleifend, purus in lacinia fermentum, est quam pharetra massa, vel euismod felis lectus et urna.Maecenas ultricies at libero non hendrerit.Sed ultricies ligula ut justo gravida, nec varius enim fermentum."
    "Vivamus mollis diam non tortor dapibus pellentesque.Quisque tincidunt dignissim nisl, laoreet faucibus erat scelerisque auctor.Nam accumsan ultrices elementum.Curabitur vulputate eget mi nec congue.Nulla ultricies scelerisque arcu, at sodales libero iaculis vel.Integer blandit vitae elit et scelerisque.Curabitur purus sapien, ornare vitae malesuada nec, ornare vitae dui.Nunc malesuada felis ac turpis pharetra lobortis.Duis imperdiet blandit mollis.Integer eget sodales risus."
    "Phasellus ac risus ac turpis convallis viverra nec sit amet erat.Ut non libero id lacus laoreet tincidunt condimentum eu turpis.Vivamus viverra ullamcorper erat eget auctor.Duis tincidunt, libero eu faucibus vehicula, risus lectus tristique ex, non porttitor ex urna ut libero.Aliquam finibus tortor in eleifend porta.Pellentesque eget felis cursus, lacinia arcu eu, molestie est.Etiam et justo nulla.In vitae eleifend dolor, vehicula vestibulum neque.Proin ac facilisis lacus.Cras posuere gravida ex, nec egestas purus imperdiet nec.Etiam nec tortor quis nunc luctus pretium.Aenean aliquet lobortis ligula quis suscipit.In id orci consequat, finibus dolor non, hendrerit felis."
    "Suspendisse bibendum, sapien porta vehicula molestie, sem justo vehicula massa, non consectetur sem neque et felis.Donec quis mauris eget augue sodales sodales.Aenean placerat at dolor at pretium.Duis congue luctus justo a efficitur.Proin molestie urna nunc, vehicula ultrices enim auctor ut.Vestibulum egestas mi elit, at gravida mi hendrerit et.Proin scelerisque efficitur dolor, ut ultrices ex venenatis vehicula.Morbi placerat venenatis nisl nec imperdiet.Aliquam in vestibulum est, ac sodales purus.Proin vel iaculis lectus.Cras in blandit nulla, a posuere ante.Vestibulum finibus neque quis laoreet cursus."
    "Phasellus convallis elit a odio porttitor, vitae dictum orci pellentesque.Interdum et malesuada fames ac ante ipsum primis in faucibus.Nunc ullamcorper eget quam vitae tincidunt.Nam id ligula risus.Nulla facilisi.Nullam tristique dignissim est vel tempus.Etiam et elit risus.Nam efficitur ligula et ultricies vulputate.Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas.Cras ultricies nisi ligula, congue semper nunc finibus blandit.Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus."
    "Curabitur pellentesque dui quis diam sodales, at imperdiet lectus tempor.Duis vestibulum nulla non elit facilisis facilisis.Curabitur iaculis aliquam eros id efficitur.Praesent turpis tellus, ultrices et orci non, mattis euismod diam.Mauris a lobortis dui, non tempus arcu.Mauris cursus nisl vel molestie ultricies.Nulla sit amet placerat ante.Nullam cursus vehicula metus, quis facilisis nulla cursus nec.Fusce id magna tortor.Praesent placerat dignissim congue."
    "Aliquam hendrerit ullamcorper nibh sed tincidunt.Fusce ornare imperdiet augue, vel vehicula nulla tincidunt eu.Pellentesque nec ipsum non leo sagittis volutpat sit amet sit amet velit.Duis bibendum lectus quis felis aliquet, vitae posuere urna tempus.Duis ut arcu egestas, interdum orci at, ornare tortor.Nullam nec lorem ac mauris interdum ultricies.Morbi dapibus feugiat commodo.Nulla sed tortor vulputate metus tristique bibendum.Quisque ligula augue, tempus porta pretium vel, accumsan at eros.Proin maximus, justo non porttitor condimentum, augue erat tempor risus, at iaculis mauris sem sit amet dolor.Nulla facilisi.Proin risus odio, vehicula id congue id, gravida sed lacus.Nullam lacinia, mi sed elementum porttitor, augue metus tempor mi, eget feugiat sapien odio ac turpis.In dolor leo, faucibus at risus eget, posuere pharetra diam.Donec tristique velit quis diam malesuada condimentum."
    "Vivamus ut dictum massa, id pellentesque augue.Pellentesque feugiat leo eu mauris pulvinar, eu commodo orci faucibus.Vivamus faucibus felis id est rutrum auctor.Nunc dolor nisi, sagittis mollis enim in, rutrum efficitur justo.Donec leo odio, efficitur id velit tristique, semper fermentum sem.Curabitur quis libero tortor.Maecenas at faucibus erat, non porttitor lorem."
    "Aenean malesuada purus id lacus sagittis faucibus.Aliquam mattis tortor ultrices, rhoncus nibh sed, aliquam diam.Curabitur sed convallis felis, at condimentum tortor.Mauris ultricies porta justo, at aliquet elit ullamcorper eu.Pellentesque fringilla condimentum vestibulum.Sed venenatis metus in eros euismod, sed condimentum tortor congue.Ut laoreet odio nec quam efficitur congue.Etiam luctus convallis lectus, eu mattis nibh tristique at.Pellentesque felis leo, consequat nec posuere nec, egestas non enim.Proin ut pulvinar metus."
    "Suspendisse viverra justo arcu, nec lobortis metus condimentum in.Nunc mi orci, hendrerit vel sapien a, euismod viverra lacus.Curabitur nibh lectus, pretium nec eros ac, semper sagittis lectus.Praesent volutpat magna sed sem tempus, et bibendum sem pellentesque.Cras imperdiet ut nisi sed placerat.Nunc accumsan leo eget risus dignissim rutrum.Duis quis erat lacus.Aenean maximus lectus in dui pharetra, ut maximus ligula elementum.Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Duis id tellus quis augue maximus consectetur.Maecenas ultrices quis ligula quis sagittis.Aenean euismod risus eu odio dignissim, sit amet venenatis neque pulvinar."
    "Quisque eleifend, ex tristique sodales iaculis, velit turpis consequat justo, a sagittis justo elit ut justo.Donec ultricies, lacus imperdiet condimentum lobortis, dui augue tempor mauris, at tristique eros lacus quis turpis.Proin sed ex non mi congue auctor congue non ex.Aenean eu urna ac purus venenatis pharetra vel quis purus.Vivamus tortor ligula, pulvinar at libero ac, iaculis congue leo.Maecenas massa ex, pulvinar vitae fermentum at, molestie ut quam.Suspendisse nisi elit, porttitor nec hendrerit blandit, convallis in tortor.Vestibulum vestibulum tristique urna sit amet auctor.Donec consequat, ipsum eget imperdiet placerat, dui libero sagittis nisl, eu tristique arcu nibh nec dolor.Praesent mollis, erat eu eleifend lacinia, turpis massa imperdiet mauris, at faucibus arcu dolor at diam.Vivamus vel justo sapien.Proin euismod neque et metus pellentesque vehicula.Aenean euismod sapien non tristique ullamcorper.Curabitur tincidunt dignissim libero a imperdiet."
    "Praesent non massa luctus, gravida nulla vitae, auctor mi.Donec nec diam blandit, fermentum nunc et, viverra sapien.Praesent nisl mi, vulputate id fermentum id, mattis in erat.Quisque non est mollis, iaculis erat vel, viverra diam.Maecenas laoreet vel quam fermentum euismod.Mauris sapien tellus, volutpat ac libero sit amet, tempus tincidunt elit.Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas.Suspendisse potenti.Nulla aliquam est vel pretium feugiat.Nam quis suscipit ex, non malesuada orci."
    "Integer euismod enim sem.Nulla molestie odio sit amet pulvinar porta.Quisque feugiat dui libero.Mauris accumsan enim sit amet elementum vulputate.Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos.Nam volutpat turpis ex, nec ornare odio tincidunt vel.Pellentesque in lorem vitae sem commodo porttitor ut ut felis.Curabitur sit amet varius arcu, sit amet commodo odio.Nam turpis magna, sollicitudin sed ante et, consectetur volutpat ipsum.Nunc non eros ac erat sollicitudin volutpat.Cras eleifend neque quis fermentum egestas."
    "Interdum et malesuada fames ac ante ipsum primis in faucibus.Nam pellentesque velit id neque dignissim, ut ornare risus euismod.Pellentesque luctus justo nulla, malesuada maximus nisi aliquet nec.Fusce aliquet massa vel nibh tristique placerat.Quisque a lacus sit amet leo vestibulum semper.Duis porttitor ipsum id vehicula semper.Proin et felis quis nulla finibus luctus.Donec quis ullamcorper est.Donec pulvinar id urna nec pulvinar.Aliquam sed mauris odio.Sed et auctor lectus, ut lacinia nibh.Morbi vehicula sed ipsum at consequat."
    "Duis elementum diam aliquam semper euismod.Quisque volutpat consectetur est, nec volutpat diam vestibulum at.In et commodo lectus.Pellentesque sit amet iaculis mauris.Donec quis mauris non tortor suscipit placerat et nec ligula.Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus.Aliquam fringilla leo enim, non tincidunt lacus efficitur id.Vivamus sit amet tellus facilisis, tempus risus sit amet, semper urna.Vestibulum mollis velit id lectus elementum condimentum.Lorem ipsum dolor sit amet, consectetur adipiscing elit.Praesent tempor nibh posuere diam porttitor, eu porttitor odio ultricies.Phasellus quam lorem, semper vel sollicitudin et, bibendum id sapien."
    "Cras sed consectetur ligula.Integer consequat consequat pharetra.Sed dictum tempus purus.Nunc lobortis eu mi nec ornare.Nulla aliquam venenatis aliquam.Donec lobortis nunc quis justo euismod, id pulvinar nisi scelerisque.Proin accumsan congue faucibus.Nullam in eros neque.Fusce risus quam, rutrum sit amet auctor nec, porttitor nec sem.Cras nec nunc vitae lectus euismod dignissim ut at tortor.Nunc lobortis aliquet bibendum.Etiam purus nisi, mollis vitae ex vel, fringilla ornare ex.Sed tincidunt commodo pretium."
    "In nisl libero, placerat ac sem quis, lacinia mattis ipsum.Suspendisse scelerisque feugiat nisi, et maximus magna finibus in.Donec condimentum mattis purus, ut venenatis ante tincidunt sit amet.Vestibulum condimentum nunc pretium, efficitur lectus id, porttitor nulla.Sed tempus ultrices dolor, ac posuere tellus semper ac.Ut quis iaculis massa.Suspendisse potenti.Nullam sed ante nisl.Duis ornare lobortis pharetra.Maecenas id nunc et ante pellentesque sagittis in vel tellus.Phasellus eros nisi, accumsan id tincidunt nec, dictum id libero.Nam metus nibh, maximus vel nunc eu, malesuada ultrices risus.Nulla eu euismod elit.Fusce tincidunt, turpis lacinia maximus suscipit, tellus urna placerat odio, sit amet fringilla leo ligula a massa.In sodales eget urna a ultricies.Morbi lectus ante, blandit vitae vel."
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// All or nothing.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void allOrNothingReadingReopen(benchmark::State& state)
{
    static std::filesystem::path file{ "guide0.txt" };
    FileGuard::createFile(file);

    FileGuard::AllOrNothingGuideline::write(file, kDummyText);
    // Fills the file with the dummy text.

    for (auto _ : state)
    {
        auto result{ FileGuard::AllOrNothingGuideline::read(file) };
        benchmark::DoNotOptimize(result);
    }

    std::filesystem::remove(file);
}

static void allOrNothingWritingReopenAllocate(benchmark::State& state)
{
    static std::filesystem::path file{ "guide1.txt" };
    FileGuard::createFile(file);

    for (auto _ : state)
    {
        auto result{ FileGuard::AllOrNothingGuideline::write(file, kDummyText) };
        benchmark::DoNotOptimize(result);
    }

    std::filesystem::remove(file);
}

BENCHMARK(allOrNothingReadingReopen);
BENCHMARK(allOrNothingWritingReopenAllocate);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// All or nothing.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Append.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void appendReading(benchmark::State& state)
{
    static std::filesystem::path file{ "guide2.txt" };
    FileGuard::createFile(file);

    FileGuard::AppendGuideline append(file, kDummyText);
    // Fills the file with the dummy text.

    for (auto _ : state)
    {
        auto result{ append.readAndRestore(kDummyText.size()) };
        benchmark::DoNotOptimize(result);
    }

    append.close();
    std::filesystem::remove(file);
}

static void appendWritingAllocate(benchmark::State& state)
{
    static std::filesystem::path file{ "guide3.txt" };
    FileGuard::createFile(file);

    FileGuard::AppendGuideline append{ file };

    for (auto _ : state)
    {
        auto result{ append.append(kDummyText) };
        benchmark::DoNotOptimize(result);
    }

    append.close();
    std::filesystem::remove(file);
}

static void appendReadingReopen(benchmark::State& state)
{
    static std::filesystem::path file{ "guide4.txt" };
    FileGuard::createFile(file);

    FileGuard::AppendGuideline append(file, kDummyText);
    append.close();

    for (auto _ : state)
    {
        auto result{ append.open(file) };
        auto result1{ append.readAndClose(kDummyText.size()) };
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(result1);
    }

    append.close();
    std::filesystem::remove(file);
}

static void appendWritingReopenAllocate(benchmark::State& state)
{
    static std::filesystem::path file{ "guide5.txt" };
    FileGuard::createFile(file);

    FileGuard::AppendGuideline append{};

    for (auto _ : state)
    {
        auto result{ append.openAndAppend(file, kDummyText) }; // Close the file.
        auto result1{ append.close() }; // Close the file.
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(result1);
    }

    append.close();
    std::filesystem::remove(file);
}

BENCHMARK(appendReading);
BENCHMARK(appendWritingAllocate);
BENCHMARK(appendReadingReopen);
BENCHMARK(appendWritingReopenAllocate);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Append.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Indexed.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void indexedReadingManyCall(benchmark::State& state)
{
    static std::filesystem::path file{ "guide6.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<kDummyText.size()> fillTheFile{ file };
    fillTheFile.write(kDummyText, 0); // Write the dummy text at the beginning of the file.
    fillTheFile.close();

    FileGuard::IndexedGuideline<71> indexed{ file };
    // kDummyText is of size 16'117, which is 227*71. 227 calls to write() will be made.

    for (auto _ : state)
    {
        for (size_t i{ 0 }; i < kDummyText.size(); i += 71)
        {
            auto result{ indexed.read(i, 1) }; // Close the file.
            benchmark::DoNotOptimize(result);
        }
    }

    indexed.close();
    std::filesystem::remove(file);
}

static void indexedWritingManyCall(benchmark::State& state)
{
    static std::filesystem::path file{ "guide7.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<71> indexed{ file };
    // kDummyText is of size 16'117, which is 227*71. 227 calls to write() will be made.

    for (auto _ : state)
    {
        for (size_t i{ 0 }; i < kDummyText.size(); i += 71)
        {
            auto result{ indexed.write(kDummyText.substr(i, 71), i) };
            benchmark::DoNotOptimize(result);
        }
    }

    indexed.close();
    std::filesystem::remove(file);
}


static void indexedReadingManyBlocks(benchmark::State& state)
{
    static std::filesystem::path file{ "guide8.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<kDummyText.size()> fillTheFile{ file };
    fillTheFile.write(kDummyText, 0); // Write the dummy text at the beginning of the file.
    fillTheFile.close();

    FileGuard::IndexedGuideline<71> indexed{ file }; // 227 blocks at once.

    for (auto _ : state)
    {
        auto result{ indexed.read(0, 227) }; // Close the file.
        benchmark::DoNotOptimize(result);
    }

    indexed.close();
    std::filesystem::remove(file);
}

static void indexedWritingManyBlocks(benchmark::State& state)
{
    static std::filesystem::path file{ "guide9.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<71> indexed{ file }; // 227 blocks at once.

    for (auto _ : state)
    {
        auto result{ indexed.write(kDummyText, 0) };
        benchmark::DoNotOptimize(result);
    }

    indexed.close();
    std::filesystem::remove(file);
}

static void indexedReadingOneCall(benchmark::State& state)
{
    static std::filesystem::path file{ "guide10.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<kDummyText.size()> fillTheFile{ file };
    fillTheFile.write(kDummyText, 0); // Write the dummy text at the beginning of the file.
    fillTheFile.close();

    FileGuard::IndexedGuideline<kDummyText.size()> indexed{ file };

    for (auto _ : state)
    {
        auto result{ indexed.read(0, 1) };
        benchmark::DoNotOptimize(result);
    }

    indexed.close();
    std::filesystem::remove(file);
}

static void indexedWritingOneCall(benchmark::State& state)
{
    static std::filesystem::path file{ "guide11.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<kDummyText.size()> indexed{ file };

    for (auto _ : state)
    {
        auto result{ indexed.write(kDummyText, 0) };
        benchmark::DoNotOptimize(result);
    }

    indexed.close();
    std::filesystem::remove(file);
}


static void indexedReadingOneCallReopen(benchmark::State& state)
{
    static std::filesystem::path file{ "guide12.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<kDummyText.size()> indexed{ file };
    indexed.write(kDummyText, 0); // Write the dummy text at the beginning of the file.
    indexed.close();

    for (auto _ : state)
    {
        auto result{ indexed.open(file) };
        auto result1{ indexed.read(0, 1) };
        auto result2{ indexed.close() };
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(result1);
        benchmark::DoNotOptimize(result2);
    }

    indexed.close();
    std::filesystem::remove(file);
}

static void indexedWritingOneCallReopen(benchmark::State& state)
{
    static std::filesystem::path file{ "guide13.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<kDummyText.size()> indexed{};

    for (auto _ : state)
    {
        auto result{ indexed.open(file)};
        auto result1{ indexed.write(kDummyText, 0) };
        auto result2{ indexed.close() };
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(result1);
        benchmark::DoNotOptimize(result2);
    }

    indexed.close();
    std::filesystem::remove(file);
}

static void indexedWritingOneCallAllocate(benchmark::State& state)
{
    static std::filesystem::path file{ "guide11.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<kDummyText.size()> indexed{ file };

    int startIndex{ 0 };
    for (auto _ : state)
    {
        auto result{ indexed.write(kDummyText, startIndex) };
        benchmark::DoNotOptimize(result);
        startIndex += 227;
    }

    indexed.close();
    std::filesystem::remove(file);
}

static void indexedWritingOneCallReopenAllocate(benchmark::State& state)
{
    static std::filesystem::path file{ "guide13.txt" };
    FileGuard::createFile(file);

    FileGuard::IndexedGuideline<kDummyText.size()> indexed{};

    int startIndex{ 0 };
    for (auto _ : state)
    {
        auto result{ indexed.open(file)};
        auto result1{ indexed.write(kDummyText, startIndex) };
        auto result2{ indexed.close() };
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(result1);
        benchmark::DoNotOptimize(result2);

        startIndex += 227;
    }

    indexed.close();
    std::filesystem::remove(file);
}

BENCHMARK(indexedReadingManyCall);
BENCHMARK(indexedWritingManyCall);
BENCHMARK(indexedReadingManyBlocks);
BENCHMARK(indexedWritingManyBlocks);
BENCHMARK(indexedReadingOneCall);
BENCHMARK(indexedWritingOneCall);
BENCHMARK(indexedReadingOneCallReopen);
BENCHMARK(indexedWritingOneCallReopen);
BENCHMARK(indexedWritingOneCallAllocate);
BENCHMARK(indexedWritingOneCallReopenAllocate);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Indexed.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////